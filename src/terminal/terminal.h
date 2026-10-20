#ifndef TERMINAL_TERMINAL_H
#define TERMINAL_TERMINAL_H

#include <stdbool.h>
#include <stddef.h>
#include <termios.h>

/*
 * Records everything the editor needs to set up, modify, and later restore the
 * terminal.
 *
 *   - original_attributes:   a snapshot of the terminal's settings as we found
 *                            them, saved so they can be put back on exit.
 *   - initialized:           true once terminal_init() has succeeded.
 *   - raw_mode_enabled:      true while "raw mode" is active.
 *   - alternate_screen_enabled: true while the alternate screen is active.
 *   - autowrap_disabled:     true while line auto-wrapping has been turned off.
 *   - rows:                  the terminal height in text rows (owned by the
 *                            Terminal Interface; populated by terminal_init).
 *   - columns:               the terminal width in text columns (same owner).
 */
typedef struct
{
    struct termios original_attributes;
    bool initialized;
    bool raw_mode_enabled;
    bool alternate_screen_enabled;
    bool autowrap_disabled;
    size_t rows;
    size_t columns;
} TerminalState;

/*
 * Prepare the terminal for use, saving its current settings and querying its
 * dimensions.
 *
 * On success the terminal's text size is stored in `terminal->rows` and
 * `terminal->columns`.
 *
 * Parameters:
 *   terminal - the state to initialize; must not be NULL.
 *
 * Returns:
 *   true on success, false if `terminal` is NULL, the settings could not be
 *   read, or the terminal dimensions could not be determined (e.g. when
 *   standard output is not a real terminal).
 */
bool terminal_init(TerminalState *terminal);

/* Query current terminal dimensions without changing TerminalState. */
bool terminal_get_size(size_t *rows, size_t *columns);

/*
 * Switch the terminal into "raw mode" so keystrokes arrive one at a time.
 *
 * Parameters:
 *   terminal - a state previously set up by terminal_init(); must not be NULL.
 *
 * Returns:
 *   true on success, false if `terminal` is NULL, not initialized, or the
 *   settings could not be applied.
 */
bool terminal_enable_raw_mode(TerminalState *terminal);

/*
 * Put the terminal back the way we found it (undo raw mode).
 *
 * Parameters:
 *   terminal - the state holding the saved settings; may be NULL.
 */
void terminal_restore(TerminalState *terminal);

/*
 * Switch the terminal to its alternate (separate) screen buffer.
 *
 * Parameters:
 *   terminal - the state to update; must not be NULL.
 *
 * Returns:
 *   true on success, false if the escape sequences could not be written.
 */
bool terminal_enter_alternate_screen(TerminalState *terminal);

/*
 * Return the terminal to its normal screen, undoing the alternate screen.
 *
 * Parameters:
 *   terminal - the state tracking whether the alternate screen is active;
 *              must not be NULL.
 */
void terminal_leave_alternate_screen(TerminalState *terminal);

#endif
