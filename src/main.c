#include "command/command_engine.h"
#include "cursor/cursor.h"
#include "document/document.h"
#include "input/input_engine.h"
#include "input/input_parser.h"
#include "renderer/renderer.h"
#include "state/editor_state.h"
#include "terminal/terminal.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>

#define ESCAPE_TIMEOUT_MICROSECONDS 50000

static bool get_terminal_size(size_t *rows, size_t *columns)
{
    struct winsize window_size;

    if (rows == NULL || columns == NULL ||
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size) == -1 ||
        window_size.ws_row == 0 || window_size.ws_col == 0)
    {
        return false;
    }

    *rows = window_size.ws_row;
    *columns = window_size.ws_col;
    return true;
}

/* Read one byte while allowing an incomplete Escape sequence to time out. */
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

static bool redraw_after_action(Renderer *renderer, EditorState *state)
{
    if (renderer == NULL || state == NULL)
    {
        return false;
    }

    if (!viewport_ensure_cursor_visible(
            &state->viewport,
            state->cursor.row,
            state->cursor.column,
            renderer->document_rows,
            renderer->screen_buffer.columns))
    {
        return false;
    }

    return renderer_render(renderer, state);
}

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
        if (!event_ready && !input_parser_feed(&parser, byte, &event))
        {
            return false;
        }

        EditorCommand command;
        bool redraw;

        if (!input_engine_translate(&event, &command))
        {
            return false;
        }
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

int main(int argc, char *argv[])
{
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
    size_t rows;
    size_t columns;
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
    if (!get_terminal_size(&rows, &columns))
    {
        goto cleanup;
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
    if (columns < 2 || !renderer_init(&renderer, rows, columns - 1))
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