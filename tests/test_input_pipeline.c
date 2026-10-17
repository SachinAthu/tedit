#include "command/command_engine.h"
#include "document/document.h"
#include "input/input_engine.h"
#include "input/input_parser.h"
#include "state/editor_state.h"

#include <assert.h>
#include <string.h>

static void feed_and_execute(InputParser *parser, EditorState *state, unsigned char byte,
                             bool *redraw, bool *should_quit)
{
    InputEvent event;
    EditorCommand command;

    assert(input_parser_feed(parser, byte, &event));
    assert(input_engine_translate(&event, &command));
    assert(command_engine_execute(state, &command, NULL, redraw, should_quit));
}

static void test_end_to_end_input_pipeline(void)
{
    InputParser parser;
    EditorState state;
    bool redraw;
    bool should_quit;
    const DocumentLine *line;

    assert(editor_state_init(&state));
    input_parser_init(&parser);

    feed_and_execute(&parser, &state, 'x', &redraw, &should_quit);
    assert(redraw && !should_quit);
    assert(state.is_dirty);
    line = document_get_line(&state.document, 0);
    assert(line != NULL && line->length == 1 && line->data[0] == 'x');
    assert(state.cursor.column == 1);

    /* An unknown CSI sequence produces no command and does not change state. */
    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    assert(!redraw && !should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    assert(!redraw && !should_quit);
    feed_and_execute(&parser, &state, 'Z', &redraw, &should_quit);
    assert(!redraw && !should_quit);
    assert(state.cursor.column == 1);

    /* A complete arrow sequence is parsed before it becomes a command. */
    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, 'D', &redraw, &should_quit);
    assert(redraw && !should_quit);
    assert(state.cursor.column == 0);

    /* Home and End terminal sequences reach document navigation commands. */
    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, 'C', &redraw, &should_quit);
    assert(redraw && state.cursor.column == 1);
    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, 'H', &redraw, &should_quit);
    assert(redraw && state.cursor.column == 0);
    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, 'F', &redraw, &should_quit);
    assert(redraw && state.cursor.column == 1);

    /* Save without an associated path starts filename entry. */
    feed_and_execute(&parser, &state, 0x13, &redraw, &should_quit);
    assert(redraw && !should_quit);
    assert(state.is_dirty);
    assert(state.filename_entry_active);
    assert(strcmp(state.message, "Enter: save  Esc: cancel") == 0);

    /* Escape is recognized after the parser's standalone-key timeout. */
    InputEvent event;
    EditorCommand command;
    assert(input_parser_feed(&parser, 0x1B, &event));
    assert(event.type == INPUT_EVENT_NONE);
    assert(input_parser_timeout(&parser, &event));
    assert(event.type == INPUT_EVENT_ESCAPE);
    assert(input_engine_translate(&event, &command));
    assert(command.type == EDITOR_COMMAND_CANCEL);
    assert(command_engine_execute(&state, &command, NULL, &redraw, &should_quit));
    assert(redraw && !should_quit && !state.filename_entry_active);
    assert(state.is_dirty);

    feed_and_execute(&parser, &state, 0x11, &redraw, &should_quit);
    assert(redraw && !should_quit && state.quit_confirmation_active);
    assert(strstr(state.message, "Save changes before quitting?") != NULL);
    feed_and_execute(&parser, &state, 'n', &redraw, &should_quit);
    assert(!redraw && should_quit && !state.quit_confirmation_active);

    editor_state_destroy(&state);
}

int main(void)
{
    test_end_to_end_input_pipeline();
    return 0;
}
