#include "terminal.h"

#include <string.h> /* Gives us memset(), used to zero out the struct. */
#include <unistd.h> /* Gives us STDIN_FILENO, the number that names the keyboard input stream. */

/*
 * Prepares the terminal before we start using it.
 * "Init" is short for "initialize".
 *
 * Parameters:
 *   terminal (TerminalState *): pointer to the state struct to fill in.
 *                               Must not be NULL.
 *
 * Returns:
 *   bool: true  -> settings were read and the terminal is ready to use.
 *         false -> terminal was NULL, or the settings could not be read.
 */
bool terminal_init(TerminalState *terminal)
{
    /* A *pointer can be NULL (point to nothing). Writing through a NULL
     * pointer would crash, so we check for it first. */
    if (terminal == NULL)
    {
        return false;
    }

    /* Overwrite everything the struct points at with zero bytes.
     * sizeof(*terminal) is the size of the struct itself.
     * This clears both flags (initialized, raw_mode_enabled) to false (0). */
    memset(terminal, 0, sizeof(*terminal));

    /* Ask the OS for the terminal's current settings and store them.
     * tcgetattr = "terminal control get attributes".
     * STDIN_FILENO is the standard input (the keyboard).
     * It returns -1 if it fails, e.g. when not running in a real terminal.
     * The & before terminal->original_attributes passes the address so the
     * function can fill it in. */
    if (tcgetattr(STDIN_FILENO, &terminal->original_attributes) == -1)
    {
        return false;
    }

    /* Remember that init succeeded, so later functions are allowed to run. */
    terminal->initialized = true;

    return true;
}

/*
 * Switches the terminal into "raw mode".
 *
 * Normally a terminal waits for you to press Enter (line mode) and echoes what
 * you type. In raw mode we receive every key press immediately and the terminal
 * stays silent, which is what an editor needs.
 *
 * Parameters:
 *   terminal (TerminalState *): pointer to a state previously set up by
 *                               terminal_init(). Must not be NULL.
 *
 * Returns:
 *   bool: true  -> raw mode is now enabled.
 *         false -> terminal was NULL, not initialized, or the settings could
 *                  not be applied.
 */
bool terminal_enable_raw_mode(TerminalState *terminal)
{
    /* Only proceed if we have a valid state that was successfully initialized. */
    if (terminal == NULL || !terminal->initialized)
    {
        return false;
    }

    /* Start from a copy of the saved original settings, then change only the
     * parts we care about. Never edit original_attributes directly, because we
     * still need it later to restore the terminal. */
    struct termios raw_attributes = terminal->original_attributes;

    /* c_iflag controls how incoming (typed) bytes are handled.
     * The ~ flips every bit, and &= keeps all bits except the listed ones,
     * so this turns each named behavior OFF. */
    raw_attributes.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /*   BRKINT: don't send an interrupt signal on a "break" condition.
     *   ICRNL:  don't turn a carriage return (\r) into a newline (\n).
     *   INPCK:  turn off input parity checking.
     *   ISTRIP: don't strip the 8th bit, so we can read all byte values.
     *   IXON:   turn off Ctrl-S / Ctrl-Q flow control. */

    /* c_oflag controls output processing. Turning off OPOST ("output post
     * processing") stops the terminal from rewriting our output, e.g. it will
     * no longer convert "\n" into "\r\n". */
    raw_attributes.c_oflag &= ~(OPOST);

    /* c_cflag controls hardware/character settings. CS8 means "use 8 bits per
     * character" (a full byte). |= turns that option ON. */
    raw_attributes.c_cflag |= CS8;

    /* c_lflag controls local, i.e. high-level, terminal behavior.
     * Again &= ~ turns each named feature OFF. */
    raw_attributes.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /*   ECHO:   don't print the keys we type back to the screen.
     *   ICANON: turn off canonical/line mode, so input arrives key-by-key.
     *   IEXTEN: turn off extended input handling (e.g. Ctrl-V).
     *   ISIG:   don't turn Ctrl-C / Ctrl-Z into signals. */

    /* c_cc holds control characters. VMIN is the minimum number of bytes a
     * read() must wait for: 1 means "return as soon as one key is pressed". */
    raw_attributes.c_cc[VMIN] = 1;

    /* VTIME is the wait timeout in tenths of a second: 0 means "do not time
     * out", so read() blocks until VMIN bytes arrive. */
    raw_attributes.c_cc[VTIME] = 0;

    /* Apply the modified settings to the input stream.
     * tcsetattr = "terminal control set attributes".
     * TCSAFLUSH means: wait until pending output is sent, then throw away any
     * typed input that has not been read yet. Returns -1 on failure. */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_attributes) == -1)
    {
        return false;
    }

    /* Record that we are in raw mode, so terminal_restore knows to undo it. */
    terminal->raw_mode_enabled = true;

    return true;
}

/*
 * Puts the terminal back the way we found it.
 * Always call this before the program exits, otherwise the user's shell may
 * be left in a broken (silent, non-echoing) state.
 *
 * Parameters:
 *   terminal (TerminalState *): pointer to the state holding the saved
 *                               original settings. NULL is allowed.
 *
 * Returns:
 *   void: nothing. The result of restoring is intentionally ignored, and the
 *         function simply returns early if there is nothing to restore.
 */
void terminal_restore(TerminalState *terminal)
{
    /* Do nothing if there is no state, init never succeeded, or we never
     * actually switched to raw mode. */
    if (terminal == NULL || !terminal->initialized || !terminal->raw_mode_enabled)
    {
        return;
    }

    /* Reapply the original saved settings.
     * TCIFLUSH means: discard any typed input we have not read yet.
     * (void) tells the compiler we are intentionally ignoring the return value. */
    (void)tcsetattr(STDIN_FILENO, TCIFLUSH, &terminal->original_attributes);

    /* Update our flag to match reality: raw mode is now off. */
    terminal->raw_mode_enabled = false;
}
