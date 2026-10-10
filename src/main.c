#include "terminal/terminal.h"
#include "state/editor_state.h"
#include "renderer/renderer.h"

#include <unistd.h> /* Gives us read() and STDIN_FILENO (the keyboard input stream). */
#include <stdbool.h> /* Gives us bool, true, and false. */
#include <sys/ioctl.h> /* Gives us ioctl(), struct winsize, and TIOCGWINSZ. */
#include <errno.h> /* Gives us errno and EINTR, so we can retry interrupted reads. */
#include <stddef.h> /* Gives us size_t and NULL. */

/* The byte the terminal sends when the user presses Ctrl-Q.
 * 0x11 is hexadecimal for 17, which is Ctrl-Q's ASCII control code.
 * A #define is a text replacement: everywhere CONTROL_Q appears, the compiler
 * substitutes 0x11. Using a name instead of a bare number makes it readable. */
#define CONTROL_Q 0x11

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
    while (true)
    {
        if (state == NULL || renderer == NULL)
        {
            return false;
        }

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
        if (bytes_read == 0)
        {
            return true;
        }

        /* Ctrl-Q is the temporary command that quits the editor. */
        if (input == CONTROL_Q)
        {
            return true;
        }

        /*
         * Accept printable ASCII characters only.
         */
        if (input < 0x20 || input > 0x7E)
        {
            continue;
        }

        /*
         * Keep the Cursor visible until viewport
         * scrolling is implemented.
         */
        if (renderer->screen_buffer.columns < 2 || state->cursor.column >= renderer->screen_buffer.columns - 1)
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
