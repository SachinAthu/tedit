#include "command/command_engine.h"
#include "cursor/cursor.h"
#include "document/document.h"
#include "input/input_engine.h"
#include "input/input_parser.h"
#include "renderer/renderer.h"
#include "state/editor_state.h"
#include "terminal/terminal.h"

#include <errno.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

/*
 * How long to wait for the bytes that may follow an ESC key press before
 * deciding the user pressed Escape alone. Long enough for terminals to
 * deliver the rest of an arrow-key sequence, short enough that a lone ESC
 * press still feels instant.
 */
#define ESCAPE_TIMEOUT_MICROSECONDS 50000

/*
 * Read a single byte from standard input, waiting at most
 * ESCAPE_TIMEOUT_MICROSECONDS for it to arrive.
 *
 * Used only while the Input Parser is inside an incomplete Escape sequence.
 * Arrow keys and friends arrive as several bytes, but a lone ESC key is
 * followed by nothing; the timeout lets us report it as an Escape event
 * instead of blocking forever.
 *
 * Parameters:
 *   byte - out-parameter: receives the byte read on success.
 *
 * Returns:
 *   1 if one byte was read, 0 if the wait timed out or the input stream
 *   closed, -1 on a read error. `byte` is written only when 1 is returned.
 */
static int read_escape_byte(unsigned char *byte)
{
    fd_set read_set;
    struct timeval timeout;
    int result;

    FD_ZERO(&read_set);
    FD_SET(STDIN_FILENO, &read_set);
    timeout.tv_sec = 0;
    timeout.tv_usec = ESCAPE_TIMEOUT_MICROSECONDS;

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
 * Bring the cursor back into view (scrolling the Viewport if needed) and
 * redraw the whole screen.
 *
 * Called after every command that may have moved the cursor or changed the
 * document, so what is shown always matches the editor state.
 *
 * Parameters:
 *   renderer - the renderer that owns the off-screen frame buffer.
 *   state    - the current editor state to scroll and draw; not modified.
 *
 * Returns:
 *   true if the frame was drawn, false on NULL arguments or if scrolling or
 *   rendering failed.
 */
static bool redraw_after_action(Renderer *renderer, EditorState *state)
{
    if (renderer == NULL || state == NULL)
    {
        return false;
    }

    size_t cursor_visual_column;
    if (!renderer_cursor_visual_column(state, &cursor_visual_column) ||
        !viewport_ensure_cursor_visible(
            &state->viewport,
            state->cursor.row,
            cursor_visual_column,
            renderer->document_rows,
            renderer->screen_buffer.columns))
    {
        return false;
    }

    return renderer_render(renderer, state);
}

/*
 * Run the editor's main loop until the user quits.
 *
 * Each iteration performs the whole pipeline: read one byte, let the Input
 * Parser turn bytes into a key event, translate that event into a command,
 * execute the command against the editor state, and redraw when anything
 * changed.
 *
 * Parameters:
 *   state     - the editor state to operate on.
 *   renderer  - the renderer used to redraw after each command.
 *   save_path - path passed on the command line; used as the save target
 *               when the document has no file path of its own yet. May be
 *               NULL.
 *
 * Returns:
 *   true when the user quit normally, false on an unrecoverable error.
 */
static bool application_event_loop(EditorState *state, Renderer *renderer, const char *save_path)
{
    InputParser parser;
    input_parser_init(&parser);

    while (true)
    {
        unsigned char byte;
        int read_result;
        InputEvent event;
        bool should_quit;
        bool event_ready = false;

        /*
         * While the parser is in the middle of an Escape sequence, the next
         * byte must arrive quickly or not at all: read with a timeout so a
         * lone ESC press is handled promptly. Otherwise a plain blocking
         * read is enough.
         */
        if (input_parser_is_waiting(&parser))
        {
            read_result = read_escape_byte(&byte);

            if (read_result == 0)
            {
                if (!input_parser_timeout(&parser, &event))
                {
                    return false;
                }

                if (event.type == INPUT_EVENT_NONE)
                {
                    continue;
                }

                event_ready = true;
                read_result = 1;
            }
        }
        else
        {
            const ssize_t bytes_read = read(STDIN_FILENO, &byte, sizeof(byte));

            if (bytes_read < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                return false;
            }

            if (bytes_read == 0)
            {
                return true;
            }
            read_result = 1;
        }

        if (read_result < 0)
        {
            return false;
        }
        if (read_result == 0)
        {
            return true;
        }

        /*
         * Feed the byte into the parser; it returns true once a complete key
         * event is ready (or when a timeout already produced one above).
         */
        if (!event_ready && !input_parser_feed(&parser, byte, &event))
        {
            return false;
        }

        EditorCommand command;
        bool redraw;

        /* Pipeline step by step: event -> command -> state change -> redraw. */
        if (!input_engine_translate(&event, &command))
        {
            return false;
        }

        command.page_rows = renderer->document_rows;

        if (!command_engine_execute(state, &command, save_path, &redraw, &should_quit))
        {
            return false;
        }
        if (redraw && !redraw_after_action(renderer, state))
        {
            return false;
        }
        if (should_quit)
        {
            return true;
        }
    }
}

/*
 * Program entry point.
 *
 * Sets up the terminal (raw mode + alternate screen), initializes the editor
 * state, optionally loads the file named on the command line, runs the event
 * loop, and then restores everything - including after errors - before
 * returning to the shell.
 *
 * Parameters:
 *   argc - number of command-line arguments.
 *   argv - argument vector; argv[1] may name a file to open.
 *
 * Returns:
 *   0 on a clean exit, 1 on any startup or runtime error.
 */
int main(int argc, char *argv[])
{
    (void)setlocale(LC_CTYPE, "");

    if (argc > 2)
    {
        fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
        return 1;
    }

    const char *startup_path = argc == 2 ? argv[1] : NULL;
    bool startup_load_failed = false;
    TerminalState terminal;
    EditorState state;
    Renderer renderer;
    bool state_initialized = false;
    bool renderer_initialized = false;
    int exit_status = 1;

    if (!terminal_init(&terminal))
    {
        return 1;
    }
    if (!terminal_enable_raw_mode(&terminal))
    {
        terminal_restore(&terminal);
        return 1;
    }
    if (!terminal_enter_alternate_screen(&terminal))
    {
        terminal_restore(&terminal);
        return 1;
    }
    if (!editor_state_init(&state))
    {
        goto cleanup;
    }
    state_initialized = true;

    if (startup_path != NULL && !editor_state_load_file(&state, startup_path))
    {
        startup_load_failed = true;
        goto cleanup;
    }
    if (terminal.columns < 2 || !renderer_init(&renderer, terminal.rows, terminal.columns - 1))
    {
        goto cleanup;
    }
    renderer_initialized = true;

    if (!redraw_after_action(&renderer, &state) || !application_event_loop(&state, &renderer, startup_path))
    {
        goto cleanup;
    }

    exit_status = 0;

cleanup:
    if (renderer_initialized)
    {
        renderer_destroy(&renderer);
    }
    if (state_initialized)
    {
        editor_state_destroy(&state);
    }
    terminal_restore(&terminal);
    terminal_leave_alternate_screen(&terminal);

    if (startup_load_failed)
    {
        fprintf(stderr, "tedit: unable to open or decode '%s'.\n", startup_path);
    }

    return exit_status;
}