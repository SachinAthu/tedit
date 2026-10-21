#define _POSIX_C_SOURCE 200809L

#include "command/command_engine.h"
#include "common/logger.h"
#include "cursor/cursor.h"
#include "document/document.h"
#include "input/input_engine.h"
#include "input/input_parser.h"
#include "renderer/renderer.h"
#include "state/editor_state.h"
#include "terminal/terminal.h"

#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

/*
 * How long to wait for the bytes that may follow an ESC key press before
 * deciding the user pressed Escape alone. Long enough for terminals to
 * deliver the rest of an arrow-key sequence, short enough that a lone ESC
 * press still feels instant.
 */
#define ESCAPE_TIMEOUT_MICROSECONDS 50000

/* Signal handlers only set flags; all terminal and allocation work remains in
 * the ordinary event-loop context, never inside an asynchronous handler. */
static volatile sig_atomic_t resize_requested = 0;
static volatile sig_atomic_t termination_requested = 0;
static struct sigaction previous_winch_action;
static struct sigaction previous_int_action;
static struct sigaction previous_term_action;
static struct sigaction previous_hup_action;
static bool signal_handlers_installed = false;

/*
 * Return a human-readable name for an input event, used only in log output.
 * Unknown values are reported as "unknown" so a bad enum value never crashes
 * a logging call.
 */
static const char *application_input_event_name(InputEventType type)
{
    switch (type)
    {
    case INPUT_EVENT_NONE:
        return "none";
    case INPUT_EVENT_CHARACTER:
        return "character";
    case INPUT_EVENT_CTRL_Q:
        return "ctrl-q";
    case INPUT_EVENT_CTRL_S:
        return "ctrl-s";
    case INPUT_EVENT_BACKSPACE:
        return "backspace";
    case INPUT_EVENT_ENTER:
        return "enter";
    case INPUT_EVENT_ESCAPE:
        return "escape";
    case INPUT_EVENT_ARROW_LEFT:
        return "left";
    case INPUT_EVENT_ARROW_RIGHT:
        return "right";
    case INPUT_EVENT_ARROW_UP:
        return "up";
    case INPUT_EVENT_ARROW_DOWN:
        return "down";
    case INPUT_EVENT_HOME:
        return "home";
    case INPUT_EVENT_END:
        return "end";
    case INPUT_EVENT_PAGE_UP:
        return "page-up";
    case INPUT_EVENT_PAGE_DOWN:
        return "page-down";
    case INPUT_EVENT_TAB:
        return "tab";
    case INPUT_EVENT_DELETE:
        return "delete";
    default:
        return "unknown";
    }
}

/*
 * Return a human-readable name for an editor command, used only in log
 * output. Unknown values are reported as "unknown".
 */
static const char *application_command_name(EditorCommandType type)
{
    switch (type)
    {
    case EDITOR_COMMAND_NONE:
        return "none";
    case EDITOR_COMMAND_INSERT_CHARACTER:
        return "insert-character";
    case EDITOR_COMMAND_INSERT_TAB:
        return "insert-tab";
    case EDITOR_COMMAND_BACKSPACE:
        return "backspace";
    case EDITOR_COMMAND_INSERT_NEWLINE:
        return "insert-newline";
    case EDITOR_COMMAND_CANCEL:
        return "cancel";
    case EDITOR_COMMAND_MOVE_LEFT:
        return "move-left";
    case EDITOR_COMMAND_MOVE_RIGHT:
        return "move-right";
    case EDITOR_COMMAND_MOVE_UP:
        return "move-up";
    case EDITOR_COMMAND_MOVE_DOWN:
        return "move-down";
    case EDITOR_COMMAND_PAGE_UP:
        return "page-up";
    case EDITOR_COMMAND_PAGE_DOWN:
        return "page-down";
    case EDITOR_COMMAND_MOVE_HOME:
        return "home";
    case EDITOR_COMMAND_MOVE_END:
        return "end";
    case EDITOR_COMMAND_DELETE_CHARACTER:
        return "delete";
    case EDITOR_COMMAND_SAVE:
        return "save";
    case EDITOR_COMMAND_QUIT:
        return "quit";
    default:
        return "unknown";
    }
}

/*
 * Shared handler for the signals we install. A signal can arrive between any
 * two machine instructions, so it must do almost nothing: it only sets a
 * `volatile sig_atomic_t` flag, which the C standard guarantees is safe to
 * modify from an asynchronous handler. The main loop checks these flags and
 * performs the actual work (resize, cleanup, exit) at a safe point.
 */
static void application_signal_handler(int signal_number)
{
    if (signal_number == SIGWINCH)
    {
        resize_requested = 1;
    }
    else
    {
        termination_requested = 1;
    }
}

/*
 * Install handlers for the signals the editor must react to: window resize
 * (SIGWINCH) and termination (SIGINT, SIGTERM, SIGHUP). The previous handler
 * for each signal is saved so it can be restored on exit. If installing one
 * handler fails, any already-installed handlers are rolled back before
 * returning false.
 */
static bool application_install_signal_handlers(void)
{
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = application_signal_handler;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGWINCH, &action, &previous_winch_action) == -1)
    {
        TEDIT_LOG_ERROR("lifecycle", "failed to install SIGWINCH handler");
        return false;
    }
    if (sigaction(SIGINT, &action, &previous_int_action) == -1)
    {
        (void)sigaction(SIGWINCH, &previous_winch_action, NULL);
        return false;
    }
    if (sigaction(SIGTERM, &action, &previous_term_action) == -1)
    {
        (void)sigaction(SIGINT, &previous_int_action, NULL);
        (void)sigaction(SIGWINCH, &previous_winch_action, NULL);
        return false;
    }
    if (sigaction(SIGHUP, &action, &previous_hup_action) == -1)
    {
        (void)sigaction(SIGTERM, &previous_term_action, NULL);
        (void)sigaction(SIGINT, &previous_int_action, NULL);
        (void)sigaction(SIGWINCH, &previous_winch_action, NULL);
        return false;
    }

    signal_handlers_installed = true;
    return true;
}

/*
 * Put back the signal handlers that were active before the editor started.
 * This is the reverse of application_install_signal_handlers() and is safe
 * to call even if no handlers were installed.
 */
static void application_restore_signal_handlers(void)
{
    if (!signal_handlers_installed)
    {
        return;
    }

    (void)sigaction(SIGHUP, &previous_hup_action, NULL);
    (void)sigaction(SIGTERM, &previous_term_action, NULL);
    (void)sigaction(SIGINT, &previous_int_action, NULL);
    (void)sigaction(SIGWINCH, &previous_winch_action, NULL);
    signal_handlers_installed = false;
}

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
 * Rebuild the Renderer for the latest terminal dimensions. A replacement is
 * allocated first, so allocation failure leaves the existing Renderer valid
 * for cleanup. The terminal dimensions are committed only after initialization
 * succeeds.
 *
 * Parameters:
 *   terminal - the terminal state whose dimensions are re-read.
 *   state    - the editor state to redraw after resizing.
 *   renderer - the renderer to replace; on success it holds the new frame
 *              buffer.
 *
 * Returns:
 *   true on success, false on NULL arguments, an unchanged or unusably small
 *   terminal size, allocation failure, or a redraw failure.
 */
static bool application_handle_resize(TerminalState *terminal, EditorState *state, Renderer *renderer)
{
    size_t rows;
    size_t columns;
    Renderer replacement;

    if (terminal == NULL || state == NULL || renderer == NULL ||
        !terminal_get_size(&rows, &columns))
    {
        return false;
    }

    if (rows == terminal->rows && columns == terminal->columns)
    {
        return true;
    }
    TEDIT_LOG_INFO("terminal", "resize requested: rows=%zu columns=%zu", rows, columns);

    /* The Renderer needs at least one document row and reserves the last
     * physical terminal column to avoid delayed-wrap behavior. */
    if (rows < 4 || columns < 2)
    {
        TEDIT_LOG_WARN("terminal", "rejected terminal size rows=%zu columns=%zu", rows, columns);
        return false;
    }

    if (!renderer_init(&replacement, rows, columns - 1))
    {
        TEDIT_LOG_ERROR("renderer", "failed to allocate replacement after terminal resize");
        return false;
    }

    Renderer previous = *renderer;
    *renderer = replacement;
    terminal->rows = rows;
    terminal->columns = columns;
    renderer_destroy(&previous);
    TEDIT_LOG_INFO("renderer", "renderer resized to rows=%zu columns=%zu", rows, columns - 1);

    if (!redraw_after_action(renderer, state))
    {
        TEDIT_LOG_ERROR("renderer", "redraw failed after terminal resize");
        return false;
    }
    return true;
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
 *   terminal  - the terminal state used to query dimensions on resize.
 *   state     - the editor state to operate on.
 *   renderer  - the renderer used to redraw after each command.
 *   save_path - path passed on the command line; used as the save target
 *               when the document has no file path of its own yet. May be
 *               NULL.
 *
 * Returns:
 *   true when the user quit normally, false on an unrecoverable error.
 */
static bool application_event_loop(TerminalState *terminal, EditorState *state, Renderer *renderer, const char *save_path)
{
    InputParser parser;
    input_parser_init(&parser);

    while (true)
    {
        if (termination_requested)
        {
            return true;
        }
        if (resize_requested)
        {
            resize_requested = 0;
            if (!application_handle_resize(terminal, state, renderer))
            {
                return false;
            }
        }

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
                    TEDIT_LOG_ERROR("input", "parser timeout handling failed");
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
                TEDIT_LOG_ERROR("input", "terminal input read failed: errno=%d", errno);
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
            TEDIT_LOG_ERROR("input", "Input Parser rejected byte 0x%02X", (unsigned int)byte);
            return false;
        }

        if (event.type != INPUT_EVENT_NONE)
        {
            if (event.type == INPUT_EVENT_CHARACTER)
            {
                TEDIT_LOG_DEBUG("input", "recognized event=%s codepoint=U+%04X", application_input_event_name(event.type), (unsigned int)event.character);
            }
            else
            {
                TEDIT_LOG_DEBUG("input", "recognized event=%s", application_input_event_name(event.type));
            }
        }

        EditorCommand command;
        bool redraw;

        /* Pipeline step by step: event -> command -> state change -> redraw. */
        if (!input_engine_translate(&event, &command))
        {
            TEDIT_LOG_ERROR("input", "Input Engine failed to translate event=%s",
                            application_input_event_name(event.type));
            return false;
        }

        if (command.type != EDITOR_COMMAND_NONE)
        {
            TEDIT_LOG_DEBUG("command", "executing command=%s", application_command_name(command.type));
        }
        command.page_rows = renderer->document_rows;

        if (!command_engine_execute(state, &command, save_path, &redraw, &should_quit))
        {
            TEDIT_LOG_ERROR("command", "Command Engine failed for command=%s", application_command_name(command.type));
            return false;
        }
        if (redraw && !redraw_after_action(renderer, state))
        {
            TEDIT_LOG_ERROR("renderer", "redraw failed after command=%s", application_command_name(command.type));
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

#ifdef TEDIT_ENABLE_LOGGING
#ifdef NDEBUG
    (void)logger_init("log.txt", LOGGER_LEVEL_INFO);
#else
    (void)logger_init("log.txt", LOGGER_LEVEL_DEBUG);
#endif
    TEDIT_LOG_INFO("lifecycle", "editor process started");
#endif

    if (argc > 2)
    {
        TEDIT_LOG_ERROR("lifecycle", "invalid command-line argument count: %d", argc);
        fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
#ifdef TEDIT_ENABLE_LOGGING
        logger_shutdown();
#endif
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

    if (!application_install_signal_handlers())
    {
        TEDIT_LOG_ERROR("lifecycle", "failed to install signal handlers");
        goto early_cleanup;
    }
    if (!terminal_init(&terminal))
    {
        TEDIT_LOG_ERROR("terminal", "terminal initialization failed");
        application_restore_signal_handlers();
        goto early_cleanup;
    }
    if (!terminal_enable_raw_mode(&terminal))
    {
        TEDIT_LOG_ERROR("terminal", "failed to enable raw mode");
        terminal_restore(&terminal);
        application_restore_signal_handlers();
        goto early_cleanup;
    }
    if (!terminal_enter_alternate_screen(&terminal))
    {
        TEDIT_LOG_ERROR("terminal", "failed to enter alternate screen");
        terminal_restore(&terminal);
        application_restore_signal_handlers();
        goto early_cleanup;
    }
    if (!editor_state_init(&state))
    {
        TEDIT_LOG_ERROR("state", "Editor State initialization failed");
        goto cleanup;
    }
    TEDIT_LOG_INFO("state", "Editor State initialized");
    state_initialized = true;

    if (startup_path != NULL && !editor_state_load_file(&state, startup_path))
    {
        startup_load_failed = true;
        TEDIT_LOG_ERROR("file", "startup file load failed: path=\"%s\"", startup_path);
        goto cleanup;
    }
    if (terminal.columns < 2 || !renderer_init(&renderer, terminal.rows, terminal.columns - 1))
    {
        TEDIT_LOG_ERROR("renderer", "initialization failed for terminal rows=%zu columns=%zu", terminal.rows, terminal.columns);
        goto cleanup;
    }
    TEDIT_LOG_INFO("renderer", "initialized rows=%zu columns=%zu", terminal.rows, terminal.columns - 1);
    renderer_initialized = true;

    if (!redraw_after_action(&renderer, &state))
    {
        TEDIT_LOG_ERROR("renderer", "initial frame rendering failed");
        goto cleanup;
    }
    if (!application_event_loop(&terminal, &state, &renderer, startup_path))
    {
        TEDIT_LOG_ERROR("lifecycle", "event loop terminated after an unrecoverable error");
        goto cleanup;
    }

    exit_status = 0;

cleanup:
    if (renderer_initialized)
    {
        renderer_destroy(&renderer);
        TEDIT_LOG_DEBUG("renderer", "Renderer resources released");
    }
    if (state_initialized)
    {
        editor_state_destroy(&state);
        TEDIT_LOG_DEBUG("state", "Editor State resources released");
    }
    terminal_restore(&terminal);
    terminal_leave_alternate_screen(&terminal);
    application_restore_signal_handlers();

    if (startup_load_failed)
    {
        fprintf(stderr, "tedit: unable to open or decode '%s'.\n", startup_path);
    }

    TEDIT_LOG_INFO("lifecycle", "editor process stopped with status=%d", exit_status);
#ifdef TEDIT_ENABLE_LOGGING
    logger_shutdown();
#endif
    return exit_status;

early_cleanup:
    TEDIT_LOG_INFO("lifecycle", "editor process stopped during startup with status=1");
#ifdef TEDIT_ENABLE_LOGGING
    logger_shutdown();
#endif
    return 1;
}
