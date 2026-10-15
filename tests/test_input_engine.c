#include "input/input_engine.h"

#include <assert.h>

static void assert_mapping(InputEventType event_type, uint32_t character,
                           EditorCommandType expected_type, uint32_t expected_character)
{
    InputEvent event = {event_type, character};
    EditorCommand command;

    assert(input_engine_translate(&event, &command));
    assert(command.type == expected_type);
    assert(command.character == expected_character);
}

static void test_all_supported_mappings(void)
{
    assert_mapping(INPUT_EVENT_NONE, 0, EDITOR_COMMAND_NONE, 0);
    assert_mapping(INPUT_EVENT_CHARACTER, 'x', EDITOR_COMMAND_INSERT_CHARACTER, 'x');
    assert_mapping(INPUT_EVENT_CTRL_Q, 0, EDITOR_COMMAND_QUIT, 0);
    assert_mapping(INPUT_EVENT_CTRL_S, 0, EDITOR_COMMAND_SAVE, 0);
    assert_mapping(INPUT_EVENT_BACKSPACE, 0, EDITOR_COMMAND_BACKSPACE, 0);
    assert_mapping(INPUT_EVENT_ENTER, 0, EDITOR_COMMAND_INSERT_NEWLINE, 0);
    assert_mapping(INPUT_EVENT_ESCAPE, 0, EDITOR_COMMAND_CANCEL, 0);
    assert_mapping(INPUT_EVENT_ARROW_LEFT, 0, EDITOR_COMMAND_MOVE_LEFT, 0);
    assert_mapping(INPUT_EVENT_ARROW_RIGHT, 0, EDITOR_COMMAND_MOVE_RIGHT, 0);
    assert_mapping(INPUT_EVENT_ARROW_UP, 0, EDITOR_COMMAND_MOVE_UP, 0);
    assert_mapping(INPUT_EVENT_ARROW_DOWN, 0, EDITOR_COMMAND_MOVE_DOWN, 0);
}

static void test_invalid_arguments_and_unknown_event(void)
{
    InputEvent event = {INPUT_EVENT_NONE, 0};
    EditorCommand command = {EDITOR_COMMAND_QUIT, 123};

    assert(!input_engine_translate(NULL, &command));
    assert(!input_engine_translate(&event, NULL));

    event.type = (InputEventType)999;
    assert(!input_engine_translate(&event, &command));
}

int main(void)
{
    test_all_supported_mappings();
    test_invalid_arguments_and_unknown_event();
    return 0;
}