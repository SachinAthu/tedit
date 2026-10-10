#ifndef TERMINAL_TERMINAL_H
#define TERMINAL_TERMINAL_H

#include <stdbool.h>
#include <termios.h>

typedef struct
{
    struct termios original_attributes;
    bool initialized;
    bool raw_mode_enabled;
    bool alternate_screen_enabled;
} TerminalState;

bool terminal_init(TerminalState *terminal);

bool terminal_enable_raw_mode(TerminalState *terminal);

void terminal_restore(TerminalState *terminal);

bool terminal_enter_alternate_screen(TerminalState *terminal);

void terminal_leave_alternate_screen(TerminalState *terminal);

#endif
