#include "input/input_engine.h"

/*
 * Fill in a command with the given type and character.
 *
 * Parameters:
 *   command   - the command to fill in; must not be NULL.
 *   type      - the command type to store.
 *   character - the character to store (used only by insert commands).
 *
 * Returns:
 *   void.
 */
static void command_set(EditorCommand *command, EditorCommandType type, uint32_t character)
{
    command->type = type;
    command->character = character;
}

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
    case INPUT_EVENT_HOME:
        command_set(command, EDITOR_COMMAND_MOVE_HOME, 0);
        break;
    case INPUT_EVENT_END:
        command_set(command, EDITOR_COMMAND_MOVE_END, 0);
        break;
    case INPUT_EVENT_PAGE_UP:
    case INPUT_EVENT_PAGE_DOWN:
    case INPUT_EVENT_TAB:
        break;
    case INPUT_EVENT_DELETE:
        command_set(command, EDITOR_COMMAND_DELETE_CHARACTER, 0);
        break;
    default:
        return false;
    }

    return true;
}
