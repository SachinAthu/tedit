#include "input/input_engine.h"

static void command_set(EditorCommand *command, EditorCommandType type, uint32_t character)
{
    command->type = type;
    command->character = character;
}

bool input_engine_translate(const InputEvent *event, EditorCommand *command)
{
    if (event == NULL || command == NULL)
    {
        return false;
    }

    command_set(command, EDITOR_COMMAND_NONE, 0);

    switch (event->type)
    {
    case INPUT_EVENT_NONE:
        break;
    case INPUT_EVENT_CHARACTER:
        command_set(command, EDITOR_COMMAND_INSERT_CHARACTER, event->character);
        break;
    case INPUT_EVENT_CTRL_Q:
        command_set(command, EDITOR_COMMAND_QUIT, 0);
        break;
    case INPUT_EVENT_CTRL_S:
        command_set(command, EDITOR_COMMAND_SAVE, 0);
        break;
    case INPUT_EVENT_BACKSPACE:
        command_set(command, EDITOR_COMMAND_BACKSPACE, 0);
        break;
    case INPUT_EVENT_ENTER:
        command_set(command, EDITOR_COMMAND_INSERT_NEWLINE, 0);
        break;
    case INPUT_EVENT_ESCAPE:
        command_set(command, EDITOR_COMMAND_CANCEL, 0);
        break;
    case INPUT_EVENT_ARROW_LEFT:
        command_set(command, EDITOR_COMMAND_MOVE_LEFT, 0);
        break;
    case INPUT_EVENT_ARROW_RIGHT:
        command_set(command, EDITOR_COMMAND_MOVE_RIGHT, 0);
        break;
    case INPUT_EVENT_ARROW_UP:
        command_set(command, EDITOR_COMMAND_MOVE_UP, 0);
        break;
    case INPUT_EVENT_ARROW_DOWN:
        command_set(command, EDITOR_COMMAND_MOVE_DOWN, 0);
        break;
    default:
        return false;
    }

    return true;
}