#ifndef INPUT_INPUT_ENGINE_H
#define INPUT_INPUT_ENGINE_H

#include "command/command_engine.h"
#include "input/input_parser.h"

#include <stdbool.h>

/*
 * Map one abstract key event to an editor command.
 *
 * This is a pure lookup: it does not touch the editor state, and it leaves
 * `command` as EDITOR_COMMAND_NONE when the event does not represent any
 * action.
 *
 * Parameters:
 *   event   - the key event to translate; must not be NULL.
 *   command - out-parameter: receives the mapped command; must not be NULL.
 *
 * Returns:
 *   true if the event was recognized, false on NULL arguments or an unknown
 *   event type.
 */
bool input_engine_translate(const InputEvent *event, EditorCommand *command);

#endif
