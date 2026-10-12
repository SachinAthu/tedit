#ifndef TERMINAL_TERMINAL_H
#define TERMINAL_TERMINAL_H

#include <stdbool.h>
#include <termios.h>

/* Tracks the terminal's configuration so it can be restored when the editor
 * exits. `original_attributes` saves the settings the shell had before we
 * switched to raw mode; the three flags record what we have changed so that
 * cleanup only undoes what was actually done. */
typedef struct
{
    struct termios original_attributes; /* Terminal settings to restore. */
    bool initialized;                   /* True after terminal_init() succeeds. */
    bool raw_mode_enabled;              /* True while raw mode is active. */
    bool alternate_screen_enabled;      /* True while the alternate screen is active. */
} TerminalState;

bool terminal_init(TerminalState *terminal);

bool terminal_enable_raw_mode(TerminalState *terminal);

void terminal_restore(TerminalState *terminal);

bool terminal_enter_alternate_screen(TerminalState *terminal);

void terminal_leave_alternate_screen(TerminalState *terminal);

#endif
