#ifndef INPUT_INPUT_ENGINE_H
#define INPUT_INPUT_ENGINE_H

#include "command/command_engine.h"
#include "input/input_parser.h"

#include <stdbool.h>

/* Map one abstract key event to an editor command without changing editor state. */
bool input_engine_translate(const InputEvent *event, EditorCommand *command);

#endif
