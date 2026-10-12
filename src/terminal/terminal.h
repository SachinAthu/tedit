#ifndef TERMINAL_TERMINAL_H
#define TERMINAL_TERMINAL_H

#include <stdbool.h>
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
 */
typedef struct
{
    struct termios original_attributes;
    bool initialized;
    bool raw_mode_enabled;
    bool alternate_screen_enabled;
    bool autowrap_disabled;
} TerminalState;

/*
 * Prepare the terminal for use, saving its current settings.
 *
 * Parameters:
 *   terminal - the state to initialize; must not be NULL.
 *
 * Returns:
 *   true on success, false if `terminal` is NULL or the settings could not be
 *   read.
 */
bool terminal_init(TerminalState *terminal);

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
