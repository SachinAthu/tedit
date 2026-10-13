#include "renderer.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * This file turns the editor's current state into characters on the screen.
 * It works in two steps:
 *   1. Build a complete "frame" (an in-memory grid of characters) from the
 *      document and cursor.
 *   2. Write that frame to the terminal using ANSI escape sequences.
 *
 * The functions below are declared "static" so they can only be used inside
 * this file. They are helper functions for renderer_render(), which is the
 * only function other files need to call.
 */

/* Fill an in-memory ScreenBuffer with the visible part of the document. */
static bool renderer_build_frame(Renderer *renderer, const EditorState *state);

/* Send a finished ScreenBuffer to the terminal. */
static bool renderer_write_frame(Renderer *renderer);

/* Grow the output buffer so it can hold at least 'required' bytes. */
static bool renderer_ensure_output_capacity(Renderer *renderer, size_t required);

/* Write all bytes to the terminal, retrying if the write is interrupted. */
static bool renderer_write_all(const char *data, size_t length);

/* Move the terminal's visible cursor to a given row and column. */
static bool renderer_write_cursor_position(size_t row, size_t column);

static size_t renderer_utf8_length(uint32_t code_point);

static size_t renderer_encode_utf8(uint32_t code_point, char output[4]);

/*
 * Prepare a Renderer object for use.
 *
 * A Renderer holds a single ScreenBuffer: the off-screen grid we draw into
 * before pushing it to the terminal. We first blank out the structure fields
 * (a safe starting point) and then ask screen_buffer_init() to allocate the
 * actual grid with the requested number of rows and columns.
 *
 * Parameters:
 *   renderer - Pointer to the Renderer to initialize.
 *   rows     - Number of screen rows the grid should have.
 *   columns  - Number of screen columns the grid should have.
 *
 * Returns:
 *   true  - The Renderer is ready to use.
 *   false - 'renderer' was NULL, or the screen buffer could not be allocated.
 */
bool renderer_init(Renderer *renderer, size_t rows, size_t columns)
{
    if (renderer == NULL)
    {
        return false;
    }

    renderer->screen_buffer.cells = NULL;
    renderer->screen_buffer.rows = 0;
    renderer->screen_buffer.columns = 0;
    renderer->output_buffer = NULL;
    renderer->output_capacity = 0;

    return screen_buffer_init(&renderer->screen_buffer, rows, columns);
}

/*
 * Release everything the Renderer allocated.
 *
 * This is the counterpart to renderer_init(). We only free the memory here;
 * we do not reset the fields, since the Renderer should not be used again
 * after being destroyed.
 *
 * Parameters:
 *   renderer - Pointer to the Renderer to clean up. May be NULL (in which
 *              case nothing happens).
 *
 * Returns:
 *   Nothing.
 */
void renderer_destroy(Renderer *renderer)
{
    if (renderer == NULL)
    {
        return;
    }

    screen_buffer_destroy(&renderer->screen_buffer);

    free(renderer->output_buffer);
    renderer->output_buffer = NULL;
    renderer->output_capacity = 0;
}

/*
 * Draw one complete frame of the editor.
 *
 * This is the main entry point used by the rest of the program. It performs
 * the full draw in three ordered steps:
 *   1. Fill the off-screen buffer from the document state.
 *   2. Write that buffer to the terminal.
 *   3. Place the visible cursor at the editor's cursor position.
 *
 * Each step must succeed before the next one runs. If any step fails we stop
 * immediately and report failure by returning false.
 *
 * Parameters:
 *   renderer - Pointer to the Renderer that owns the off-screen buffer.
 *   state    - Pointer to the current editor state (document + cursor) to
 *              draw. It is read-only and will not be modified.
 *
 * Returns:
 *   true  - The frame and cursor were written successfully.
 *   false - A NULL argument was passed, or one of the two draw steps failed.
 */
bool renderer_render(Renderer *renderer, const EditorState *state)
{
    bool success;

    if (renderer == NULL || state == NULL ||
        renderer->screen_buffer.cells == NULL ||
        state->cursor.row < state->viewport.viewport_row ||
        state->cursor.column < state->viewport.viewport_column ||
        state->cursor.row - state->viewport.viewport_row >= renderer->screen_buffer.rows ||
        state->cursor.column - state->viewport.viewport_column >= renderer->screen_buffer.columns)
    {
        return false;
    }

    if (!renderer_build_frame(renderer, state))
    {
        return false;
    }

    /*
     * Hide the terminal text cursor while drawing.
     */
    if (!renderer_write_all("\x1b[?25l", 6))
    {
        return false;
    }

    success = renderer_write_frame(renderer);

    if (success)
    {
        success = renderer_write_cursor_position(state->cursor.row - state->viewport.viewport_row, state->cursor.column - state->viewport.viewport_column);
    }

    /*
     * Always attempt to restore cursor visibility,
     * even if frame rendering fails.
     */
    if (!renderer_write_all("\x1b[?25h", 6))
    {
        success = false;
    }

    return success;
}

/*
 * Copy the visible part of the document into the off-screen ScreenBuffer.
 *
 * We start by filling the whole grid with spaces so that leftover characters
 * from a previous frame do not show through. Then we walk over each row and
 * column, copying characters from the document. The bounds checks
 * (row < ... and column < ...) make sure we never read past the end of a
 * document line or write past the edge of the screen grid.
 *
 * Parameters:
 *   renderer - Pointer to the Renderer holding the off-screen buffer to fill.
 *   state    - Pointer to the editor state to read the document from.
 *
 * Returns:
 *   true  - The visible document content was copied into the buffer.
 *   false - A NULL argument was passed, the buffer is not allocated, or
 *           setting a cell failed.
 */
static bool renderer_build_frame(Renderer *renderer, const EditorState *state)
{
    if (renderer == NULL || state == NULL)
    {
        return false;
    }

    if (renderer->screen_buffer.cells == NULL)
    {
        return false;
    }

    /* Start from a clean slate: every cell becomes a blank space. */
    screen_buffer_clear(&renderer->screen_buffer, (uint32_t)' ');

    /* Map each screen row to its corresponding logical document row. */
    for (size_t screen_row = 0; screen_row < renderer->screen_buffer.rows; screen_row++)
    {
        const size_t document_row = state->viewport.viewport_row + screen_row;
        const DocumentLine *line;

        if (document_row >= state->document.line_count)
        {
            break;
        }

        line = &state->document.lines[document_row];

        /* Horizontal scrolling maps screen columns to document columns. */
        for (size_t screen_column = 0; screen_column < renderer->screen_buffer.columns; screen_column++)
        {
            const size_t document_column = state->viewport.viewport_column + screen_column;

            if (document_column >= line->length)
            {
                break;
            }

            if (!screen_buffer_set(&renderer->screen_buffer, screen_row, screen_column, line->data[document_column]))
            {
                return false;
            }
        }
    }

    return true;
}

/*
 * Grow the renderer's output buffer so it can hold at least 'required' bytes.
 *
 * The output buffer is reused across frames so that rendering does not have to
 * allocate on every redraw. It is only enlarged, never shrunk, because the
 * terminal size rarely changes and the buffer is bounded by the screen area.
 *
 * Parameters:
 *   renderer - Pointer to the Renderer that owns the output buffer.
 *   required - Minimum number of bytes the buffer must be able to hold.
 *
 * Returns:
 *   true  - The buffer is at least 'required' bytes large.
 *   false - 'renderer' was NULL, or enlarging the buffer failed.
 */
static bool renderer_ensure_output_capacity(Renderer *renderer, size_t required)
{
    if (renderer == NULL)
    {
        return false;
    }

    if (required <= renderer->output_capacity)
    {
        return true;
    }

    char *larger = realloc(renderer->output_buffer, required);
    if (larger == NULL)
    {
        return false;
    }

    renderer->output_buffer = larger;
    renderer->output_capacity = required;

    return true;
}

/*
 * Send the finished off-screen buffer to the terminal.
 *
 * The terminal understands special "escape sequences": short strings that
 * start with the ESC character (written here as \x1b) and tell the terminal
 * to do something other than print text, such as clear the screen or move the
 * cursor. Rather than writing each escape sequence and character one at a
 * time, we assemble the whole frame into a single in-memory buffer and hand it
 * to the terminal in one write() call.
 *
 * The frame is built in two passes. The first pass measures how large the
 * finished frame will be so the output buffer can be sized exactly; the second
 * pass copies each row's cursor-position sequence and its characters into
 * place.
 *
 * Note that terminal rows and columns are 1-based, while our arrays are
 * 0-based. That is why the code adds 1 when building the sequences.
 *
 * Parameters:
 *   renderer - Pointer to the Renderer holding the frame to print and the
 *              output buffer to build it into.
 *
 * Returns:
 *   true  - Every character was written to the terminal.
 *   false - 'renderer' was NULL or its frame unallocated, or a write, format,
 *           or buffer-resize step failed.
 */
static bool renderer_write_frame(Renderer *renderer)
{
    const ScreenBuffer *buffer;
    char sequence[64];

    if (renderer == NULL)
    {
        return false;
    }

    buffer = &renderer->screen_buffer;

    if (buffer->cells == NULL)
    {
        return false;
    }

    /*
     * First pass: measure the ANSI cursor-position sequences and
     * UTF-8-encoded cell contents.
     */
    size_t frame_length = 0;

    for (size_t row = 0; row < buffer->rows; row++)
    {
        int sequence_length = snprintf(sequence, sizeof(sequence), "\x1b[%zu;1H", row + 1);

        if (sequence_length < 0 || (size_t)sequence_length >= sizeof(sequence))
        {
            return false;
        }

        if (frame_length > SIZE_MAX - (size_t)sequence_length)
        {
            return false;
        }

        frame_length += (size_t)sequence_length;

        for (size_t column = 0; column < buffer->columns; ++column)
        {
            const ScreenCell *cell = screen_buffer_get(buffer, row, column);

            if (cell == NULL)
            {
                return false;
            }

            size_t encoded_length = renderer_utf8_length(cell->code_point);

            if (encoded_length == 0 || frame_length > SIZE_MAX - encoded_length)
            {
                return false;
            }

            frame_length += encoded_length;
        }
    }

    if (!renderer_ensure_output_capacity(renderer, frame_length))
    {
        return false;
    }

    /*
     * Second pass: construct the complete terminal output.
     */
    size_t position = 0;

    for (size_t row = 0; row < buffer->rows; ++row)
    {
        int sequence_length = snprintf(sequence, sizeof(sequence), "\x1b[%zu;1H", row + 1);

        if (sequence_length < 0 || (size_t)sequence_length >= sizeof(sequence))
        {
            return false;
        }

        memcpy(renderer->output_buffer + position, sequence, (size_t)sequence_length);

        position += (size_t)sequence_length;

        for (size_t column = 0; column < buffer->columns; ++column)
        {
            const ScreenCell *cell = screen_buffer_get(buffer, row, column);

            if (cell == NULL)
            {
                return false;
            }

            char encoded[4];
            size_t encoded_length = renderer_encode_utf8(cell->code_point, encoded);

            if (encoded_length == 0)
            {
                return false;
            }

            memcpy(renderer->output_buffer + position, encoded, encoded_length);

            position += encoded_length;
        }
    }

    return renderer_write_all(renderer->output_buffer, frame_length);
}

/*
 * Write exactly 'length' bytes to standard output (the terminal).
 *
 * The write() system call is allowed to write fewer bytes than asked and can
 * also be interrupted by a signal before writing anything. To handle both
 * cases we loop until every byte has been sent. The 'written' counter tracks
 * how far we have gotten, and each iteration asks the operating system to
 * write whatever is left (data + written).
 *
 * Parameters:
 *   data   - Pointer to the first byte to write.
 *   length - How many bytes to write, starting at 'data'.
 *
 * Returns:
 *   true  - All 'length' bytes were written.
 *   false - 'data' was NULL, or the operating system reported an error.
 */
static bool renderer_write_all(const char *data, size_t length)
{
    if (data == NULL)
    {
        return false;
    }

    size_t written = 0;

    while (written < length)
    {
        ssize_t result;

        /* Ask the OS to write the remaining bytes. */
        result = write(STDOUT_FILENO, data + written, length - written);

        if (result < 0)
        {
            /* EINTR means a signal interrupted us; retrying is safe. */
            if (errno == EINTR)
            {
                continue;
            }

            return false;
        }

        /* A return value of 0 means nothing could be written; give up. */
        if (result == 0)
        {
            return false;
        }

        written += (size_t)result;
    }

    return true;
}

/*
 * Move the terminal's visible cursor to the given position.
 *
 * The escape sequence "\x1b[<row>;<column>H" tells the terminal to place the
 * cursor at that row and column. As in renderer_write_frame(), we add 1 to
 * both values because terminals count from 1 while our code counts from 0.
 *
 * Parameters:
 *   row    - Zero-based row to move the cursor to.
 *   column - Zero-based column to move the cursor to.
 *
 * Returns:
 *   true  - The cursor position sequence was written successfully.
 *   false - The position could not be formatted, or the write failed.
 */
static bool renderer_write_cursor_position(size_t row, size_t column)
{
    char sequence[64];
    int sequence_length = snprintf(sequence, sizeof(sequence), "\x1b[%zu;%zuH", row + 1, column + 1);

    if (sequence_length < 0 || (size_t)sequence_length >= sizeof(sequence))
    {
        return false;
    }

    return renderer_write_all(sequence, (size_t)sequence_length);
}

/*
 * Return the number of bytes required to encode a Unicode scalar value
 * as UTF-8. Invalid scalar values are rejected.
 */
static size_t renderer_utf8_length(uint32_t code_point)
{
    if (code_point <= UINT32_C(0x7F))
    {
        return 1;
    }

    if (code_point <= UINT32_C(0x7FF))
    {
        return 2;
    }

    if (code_point >= UINT32_C(0xD800) && code_point <= UINT32_C(0xDFFF))
    {
        return 0;
    }

    if (code_point <= UINT32_C(0xFFFF))
    {
        return 3;
    }

    if (code_point <= UINT32_C(0x10FFFF))
    {
        return 4;
    }

    return 0;
}

/*
 * Encode one Unicode scalar value into a four-byte output array.
 * The returned length is zero if the value is invalid.
 */
static size_t renderer_encode_utf8(uint32_t code_point, char output[4])
{
    size_t length = renderer_utf8_length(code_point);

    if (length == 0 || output == NULL)
    {
        return 0;
    }

    if (length == 1)
    {
        output[0] = (char)code_point;
    }
    else if (length == 2)
    {
        output[0] = (char)(UINT32_C(0xC0) | (code_point >> 6));

        output[1] = (char)(UINT32_C(0x80) | (code_point & UINT32_C(0x3F)));
    }
    else if (length == 3)
    {
        output[0] = (char)(UINT32_C(0xE0) | (code_point >> 12));

        output[1] = (char)(UINT32_C(0x80) | ((code_point >> 6) & UINT32_C(0x3F)));

        output[2] = (char)(UINT32_C(0x80) | (code_point & UINT32_C(0x3F)));
    }
    else
    {
        output[0] = (char)(UINT32_C(0xF0) | (code_point >> 18));

        output[1] = (char)(UINT32_C(0x80) | ((code_point >> 12) & UINT32_C(0x3F)));

        output[2] = (char)(UINT32_C(0x80) | ((code_point >> 6) & UINT32_C(0x3F)));

        output[3] = (char)(UINT32_C(0x80) | (code_point & UINT32_C(0x3F)));
    }

    return length;
}