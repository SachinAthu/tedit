#include "terminal/terminal.h"
#include "state/editor_state.h"
#include "renderer/renderer.h"

#include <unistd.h>    /* Gives us read() and STDIN_FILENO (the keyboard input stream). */
#include <stdbool.h>   /* Gives us bool, true, and false. */
#include <sys/ioctl.h> /* Gives us ioctl(), struct winsize, and TIOCGWINSZ. */
#include <errno.h>     /* Gives us errno and EINTR, so we can retry interrupted reads. */
#include <stddef.h>    /* Gives us size_t and NULL. */
#include <sys/select.h>

/* The byte the terminal sends when the user presses Ctrl-Q.
 * 0x11 is hexadecimal for 17, which is Ctrl-Q's ASCII control code.
 * A #define is a text replacement: everywhere CONTROL_Q appears, the compiler
 * substitutes 0x11. Using a name instead of a bare number makes it readable. */
#define CONTROL_Q 0x11
/* 0x7F is the "Delete" key, which most terminals send for Backspace. */
#define BACKSPACE 0x7F
/* 0x08 is Ctrl-H, an older/alternative Backspace code. */
#define BACKSPACE_ALT 0x08
/* 0x1B is the ESC key, which starts arrow-key and other escape sequences. */
#define ESCAPE 0x1B
/* How long (in microseconds) to wait for the rest of an escape sequence. */
#define ESCAPE_TIMEOUT 50000
/* 0x0D is the carriage return the terminal sends when Enter is pressed. */
#define ENTER 0x0D
/* 0x0A is a line feed, accepted as Enter for convenience. */
#define LINE_FEED 0x0A

/*
 * Ask the terminal how many rows and columns it currently has.
 *
 * We use ioctl() ("input/output control"), a system call for talking to
 * devices. The request TIOCGWINSZ means "get window size", so the kernel
 * fills in our 'struct winsize' with the terminal's dimensions. Because a C
 * function can only return one value, we hand back the two sizes through
 * pointer arguments (out-parameters) that the caller provides.
 *
 * Parameters:
 *   rows    - Out-parameter: receives the terminal's number of rows.
 *   columns - Out-parameter: receives the terminal's number of columns.
 *
 * Returns:
 *   true  - Both sizes were read and written through the pointers.
 *   false - Either pointer was NULL, the ioctl failed, or the terminal
 *           reported a zero size (which we treat as "unknown").
 */
static bool get_terminal_size(size_t *rows, size_t *columns)
{
    struct winsize window_size;

    /* Writing through a NULL pointer would crash, so reject bad arguments. */
    if (rows == NULL || columns == NULL)
    {
        return false;
    }

    /* Ask the kernel to fill window_size with the terminal's dimensions. */
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size) == -1)
    {
        return false;
    }

    /* A zero dimension means the size is unknown, so treat it as a failure. */
    if (window_size.ws_row == 0 || window_size.ws_col == 0)
    {
        return false;
    }

    /* Hand the sizes back through the caller's out-parameters. */
    *rows = window_size.ws_row;
    *columns = window_size.ws_col;

    return true;
}

/*
 * Read one more byte from the keyboard, but only if it arrives quickly.
 *
 * Arrow keys are not a single byte: the terminal sends an escape sequence
 * like "\x1b[A" (ESC, '[', 'A'). After seeing the ESC byte, we call this to
 * grab the next byte. We use select() to wait at most ESCAPE_TIMEOUT, so that
 * a lone ESC key press (no bytes following) is not mistaken for an arrow key.
 *
 * Parameters:
 *   byte - Out-parameter: receives the byte read from the keyboard.
 *
 * Returns:
 *   1  - one byte was read into `byte`.
 *   0  - no byte arrived within the timeout.
 *   -1 - reading failed for a reason other than a signal interruption.
 */
static int read_escape_byte(unsigned char *byte)
{
    fd_set read_set;
    struct timeval timeout;
    int result;

    FD_ZERO(&read_set);
    FD_SET(STDIN_FILENO, &read_set);
    timeout.tv_sec = 0;
    timeout.tv_usec = ESCAPE_TIMEOUT;

    do
    {
        result = select(STDIN_FILENO + 1, &read_set, NULL, NULL, &timeout);
    } while (result == -1 && errno == EINTR);

    if (result <= 0)
    {
        return result;
    }

    for (;;)
    {
        const ssize_t bytes_read = read(STDIN_FILENO, byte, sizeof(*byte));

        if (bytes_read == 1)
        {
            return 1;
        }

        if (bytes_read == 0)
        {
            return 0;
        }

        if (errno != EINTR)
        {
            return -1;
        }
    }
}

/*
 * Redraw the screen after the cursor or document has changed.
 *
 * Editing commands all end the same way: make sure the cursor is still on
 * screen (scrolling if needed), then repaint. This helper does both so the
 * main loop does not repeat the code.
 *
 * Parameters:
 *   renderer - the renderer used to draw the screen; must not be NULL.
 *   state    - the current editor state (document + cursor); must not be NULL.
 *
 * Returns:
 *   true if the cursor was kept visible and the frame was drawn, false if
 *   either pointer is NULL or one of those steps failed.
 */
static bool redraw_after_action(Renderer *renderer, const EditorState *state)
{
    if (renderer == NULL || state == NULL)
    {
        return false;
    }

    if (!viewport_ensure_cursor_visible(&state->viewport, state->cursor.row, state->cursor.column, renderer->screen_buffer.rows, renderer->screen_buffer.columns))
    {
        return false;
    }

    return renderer_render(renderer, state);
}

/*
 * The main loop of the editor.
 * It keeps reading one key at a time and stops when the user asks to quit.
 *
 * Parameters:
 *   state    - Pointer to the current editor state (document + cursor) to
 *             read and modify. It is not NULL.
 *   renderer - Pointer to the renderer to use for drawing. It is not NULL.
 *
 * Returns:
 *   bool: true if the loop completed successfully, false otherwise.
 */
static bool application_event_loop(EditorState *state, Renderer *renderer)
{
    if (state == NULL || renderer == NULL)
    {
        return false;
    }

    while (true)
    {

        /* One keystroke is a single byte. unsigned char holds 0..255, which is
         * the full range of byte values we can read. Declared inside the loop
         * so it is freshly created on every pass. */
        unsigned char input;

        /* read() waits for input, then copies up to sizeof(input) bytes into
         * &input (its address) and returns how many bytes it actually read.
         * ssize_t is a signed size type: it can be negative on error (-1).
         * const means we will not modify bytes_read after this line. */
        const ssize_t bytes_read = read(STDIN_FILENO, &input, sizeof(input));

        /* A negative result means read() failed. A signal may have interrupted
         * it, which is harmless, so retry; any other error is fatal. */
        if (bytes_read < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return false;
        }

        /* Zero bytes means end-of-input (the stream closed), so stop. */
        /* Ctrl-Q is the temporary command that quits the editor. */
        if (bytes_read == 0 || input == CONTROL_Q)
        {
            return true;
        }

        /* An ESC byte starts an escape sequence, e.g. "\x1b[D" means Left.
         * Arrow keys arrive as ESC followed by '[' and a direction letter:
         *   D = Left, C = Right, A = Up, B = Down.
         * Read the two following bytes; if they do not form such a sequence,
         * ignore the input and go back to waiting for the next key. */
        if (input == ESCAPE)
        {
            unsigned char first;
            unsigned char second;

            if (read_escape_byte(&first) != 1 || first != '[' || read_escape_byte(&second) != 1)
            {
                continue;
            }

            bool moved = false;

            if (second == 'D')
            {
                moved = cursor_move_left(&state->cursor);
            }
            else if (second == 'C')
            {
                moved = cursor_move_right(&state->cursor, &state->document);
            }
            else if (second == 'A')
            {
                moved = cursor_move_up(&state->cursor, &state->document);
            }
            else if (second == 'B')
            {
                moved = cursor_move_down(&state->cursor, &state->document);
            }

            /* Only repaint if the cursor actually moved. */
            if (moved && !redraw_after_action(renderer, state))
            {
                return false;
            }

            continue;
        }

        /* Backspace deletes the character before the cursor. If the cursor is
         * at the start of a line, it merges that line into the one above. */
        if (input == BACKSPACE || input == BACKSPACE_ALT)
        {
            if (state->cursor.column > 0)
            {
                const size_t delete_column = state->cursor.column - 1;

                if (!document_delete_character(&state->document, state->cursor.row, delete_column))
                {
                    continue;
                }

                state->cursor.column = delete_column;
            }
            else if (state->cursor.row > 0)
            {
                const size_t previous_row = state->cursor.row - 1;

                const DocumentLine *previous_line = document_get_line(&state->document, previous_row);

                if (previous_line == NULL)
                {
                    continue;
                }

                const size_t previous_length = previous_line->length;

                if (!document_merge_lines(&state->document, previous_row))
                {
                    continue;
                }

                state->cursor.row = previous_row;
                state->cursor.column = previous_length;
            }
            else
            {
                continue;
            }

            state->is_dirty = true;

            if (!redraw_after_action(renderer, state))
            {
                return false;
            }

            continue;
        }

        /* Enter splits the current line at the cursor and moves the cursor to
         * the start of the newly created line below it. */
        if (input == ENTER || input == LINE_FEED)
        {
            if (!document_split_line(&state->document, state->cursor.row, state->cursor.column))
            {
                continue;
            }

            state->cursor.row += 1;
            state->cursor.column = 0;
            state->is_dirty = true;

            if (!redraw_after_action(renderer, state))
            {
                return false;
            }

            continue;
        }

        /*
         * Accept printable ASCII characters only.
         */
        if (input < 0x20 || input > 0x7E)
        {
            continue;
        }

        /* Insert the typed character at the Cursor's position. */
        if (!document_insert_character(&state->document, state->cursor.row, state->cursor.column, (char)input))
        {
            return false;
        }

        /*
         * A successful insertion increases the line
         * length, allowing the Cursor to advance.
         */
        if (!cursor_advance(&state->cursor, &state->document))
        {
            return false;
        }

        state->is_dirty = true;

        /*
         * Render the updated Editor State.
         */
        if (!renderer_render(renderer, state))
        {
            return false;
        }
    }
}

/*
 * Program entry point. The OS calls this to start the program.
 *
 * Parameters:
 *   none.
 *
 * Returns:
 *   int: the exit status. 0 means success; any non-zero value means failure.
 */
int main(void)
{
    /* The three main objects the editor needs while running. */
    TerminalState terminal;
    EditorState state;
    Renderer renderer;

    /* Terminal dimensions, filled in by get_terminal_size() below. */
    size_t rows;
    size_t columns;

    /* "Initialized" flags remember how far setup got. The cleanup block at
     * the bottom uses them so it only frees things that were actually
     * created, and never touches an uninitialized object. */
    bool state_initialized = false;
    bool renderer_initialized = false;

    /* Start by assuming failure (non-zero). We only switch to 0 once the
     * editor has run and shut down cleanly. */
    int exit_status = 1;

    /* Set up the terminal. If this fails there is nothing to clean up, so we
     * can return immediately. */
    if (!terminal_init(&terminal))
    {
        return 1;
    }

    /* Put the terminal into "raw mode" so it passes keystrokes to us directly
     * instead of line-buffering them. On failure we restore the terminal
     * before returning, since it is already initialized. */
    if (!terminal_enable_raw_mode(&terminal))
    {
        terminal_restore(&terminal);
        return 1;
    }

    /* Enter the alternate screen. */
    if (!terminal_enter_alternate_screen(&terminal))
    {
        terminal_restore(&terminal);
        return 1;
    }

    /* From here on, any failure jumps to the shared 'cleanup' block instead
     * of returning right away. This keeps the shutdown code in one place. */
    if (!get_terminal_size(&rows, &columns))
    {
        goto cleanup;
    }

    /* Create the editor state (the document and the cursor). */
    if (!editor_state_init(&state))
    {
        goto cleanup;
    }

    /* Mark the state as created so cleanup will free it. */
    state_initialized = true;

    /* Create the renderer sized to the terminal's rows and columns. */
    if (!renderer_init(&renderer, rows, columns))
    {
        goto cleanup;
    }

    /* Mark the renderer as created so cleanup will free it. */
    renderer_initialized = true;

    /* Draw the first frame before waiting for input. */
    if (!renderer_render(&renderer, &state))
    {
        goto cleanup;
    }

    /* Hand control to the main loop until the user quits. */
    if (application_event_loop(&state, &renderer))
    {
        exit_status = 0;
    }

cleanup:
    /* Free resources in reverse order of creation, skipping any that were
     * never successfully initialized. */
    if (renderer_initialized)
    {
        renderer_destroy(&renderer);
    }

    if (state_initialized)
    {
        editor_state_destroy(&state);
    }

    /* Always restore the terminal, because we switched it to raw mode. */
    terminal_restore(&terminal);

    /* Leave the alternate screen. */
    terminal_leave_alternate_screen(&terminal);

    return exit_status;
}
