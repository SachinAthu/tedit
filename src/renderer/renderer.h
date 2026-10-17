#ifndef RENDERER_RENDERER_H
#define RENDERER_RENDERER_H

#include <stdbool.h>
#include <stddef.h>

#include "state/editor_state.h"
#include "screen_buffer.h"

/*
 * Everything needed to draw one frame of the editor to the terminal.
 *
 *   - screen_buffer: the off-screen grid of characters we draw into before
 *                    sending the whole frame to the terminal.
 *   - document_rows: document text rows; the remaining rows hold a blank
 *                    separator, Message Bar, and bottom Status Bar.
 *   - output_buffer: a reusable byte buffer that holds the frame (escape
 *                    sequences plus characters) about to be written out.
 *   - output_capacity: how many bytes output_buffer currently holds.
 */
typedef struct
{
    ScreenBuffer screen_buffer;
    size_t document_rows;
    char *output_buffer;
    size_t output_capacity;
} Renderer;

/*
 * Initialize a Renderer for a screen of the given size.
 *
 * The bottom three rows are reserved for the interface (separator, Message
 * Bar, Status Bar), so `document_rows` is three less than `rows`.
 *
 * Parameters:
 *   renderer - the renderer to initialize; must not be NULL.
 *   rows     - total screen height; must be at least 4.
 *   columns  - screen width; must be greater than 0.
 *
 * Returns:
 *   true on success, false on NULL arguments, invalid sizes, or if the
 *   screen buffer could not be allocated.
 */
bool renderer_init(Renderer *renderer, size_t rows, size_t columns);

/*
 * Release everything the renderer allocated.
 *
 * Parameters:
 *   renderer - the renderer to clean up; may be NULL.
 *
 * Returns:
 *   void.
 */
void renderer_destroy(Renderer *renderer);

/*
 * Draw one complete frame of the editor from the given state.
 *
 * The frame is built into the off-screen buffer, written to the terminal,
 * and finished by placing the visible cursor at the editor's cursor
 * position (or at the filename-entry cursor while that prompt is active).
 * The editor state is read but never modified.
 *
 * Parameters:
 *   renderer - the renderer that owns the off-screen buffer; must not be
 *              NULL.
 *   state    - the editor state to draw; must not be NULL.
 *
 * Returns:
 *   true if the frame was written, false on NULL arguments, an invalid
 *   state/buffer combination, or any write failure.
 */
bool renderer_render(Renderer *renderer, const EditorState *state);

/*
 * Convert the current logical Cursor column to its visual display column.
 * Tabs advance to the next configured tab stop. Other code points occupy one
 * display cell in this milestone; Unicode display-width handling is separate.
 *
 * Returns true on success, false for invalid state, invalid tab width, or
 * arithmetic overflow.
 */
bool renderer_cursor_visual_column(const EditorState *state, size_t *visual_column);

#endif