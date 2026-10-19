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
    command.page_rows = 3;
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

    /* Delete removes the code point at the Cursor through the full pipeline. */
    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, 'H', &redraw, &should_quit);
    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '3', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '~', &redraw, &should_quit);
    line = document_get_line(&state.document, 0);
    assert(redraw && line != NULL && line->length == 0);
    assert(state.cursor.row == 0 && state.cursor.column == 0 && state.is_dirty);

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


static void test_alternate_home_end_sequences_reach_commands(void)
{
    InputParser parser;
    EditorState state;
    bool redraw;
    bool should_quit;

    assert(editor_state_init(&state));
    input_parser_init(&parser);
    feed_and_execute(&parser, &state, 'a', &redraw, &should_quit);
    feed_and_execute(&parser, &state, 'b', &redraw, &should_quit);
    feed_and_execute(&parser, &state, 'c', &redraw, &should_quit);
    assert(state.cursor.column == 3);

    /* CSI 1~ is an alternate Home sequence and must reach Cursor navigation. */
    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '1', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '~', &redraw, &should_quit);
    assert(redraw && !should_quit && state.cursor.column == 0);

    /* CSI 8~ is an alternate End sequence. */
    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '8', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '~', &redraw, &should_quit);
    assert(redraw && !should_quit && state.cursor.column == 3);
    assert(state.is_dirty);

    editor_state_destroy(&state);
}

static void test_filename_entry_ignores_document_only_keys(void)
{
    InputParser parser;
    EditorState state;
    bool redraw;
    bool should_quit;
    const DocumentLine *line;

    assert(editor_state_init(&state));
    input_parser_init(&parser);
    feed_and_execute(&parser, &state, 0x13, &redraw, &should_quit);
    assert(redraw && state.filename_entry_active);

    /* These commands belong to document editing, not the filename prompt. */
    feed_and_execute(&parser, &state, '\t', &redraw, &should_quit);
    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, 'H', &redraw, &should_quit);
    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, 'F', &redraw, &should_quit);
    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '5', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '~', &redraw, &should_quit);
    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '6', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '~', &redraw, &should_quit);

    line = document_get_line(&state.document, 0);
    assert(state.filename_entry_active);
    assert(state.filename_entry_length == 0 && state.filename_entry_cursor == 0);
    assert(line != NULL && line->length == 0);
    assert(!state.is_dirty);
    assert(!redraw && !should_quit);

    /* Complete the Escape timeout path and cancel the filename prompt. */
    InputEvent event;
    EditorCommand command;
    assert(input_parser_feed(&parser, 0x1B, &event));
    assert(event.type == INPUT_EVENT_NONE);
    assert(input_parser_timeout(&parser, &event));
    assert(event.type == INPUT_EVENT_ESCAPE);
    assert(input_engine_translate(&event, &command));
    assert(command_engine_execute(&state, &command, NULL, &redraw, &should_quit));
    assert(redraw && !should_quit && !state.filename_entry_active);
    assert(!state.is_dirty);

    editor_state_destroy(&state);
}

static void test_tab_insertion_pipeline(void)
{
    InputParser parser;
    EditorState state;
    bool redraw;
    bool should_quit;
    const DocumentLine *line;

    assert(editor_state_init(&state));
    input_parser_init(&parser);
    feed_and_execute(&parser, &state, 'x', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '\t', &redraw, &should_quit);

    line = document_get_line(&state.document, 0);
    assert(redraw && !should_quit && line != NULL && line->length == 2);
    assert(line->data[0] == (uint32_t)'x' && line->data[1] == (uint32_t)'\t');
    assert(state.cursor.column == 2 && state.is_dirty);

    editor_state_destroy(&state);
}

static void test_page_navigation_pipeline(void)
{
    InputParser parser;
    EditorState state;
    bool redraw;
    bool should_quit;

    assert(editor_state_init(&state));
    input_parser_init(&parser);

    for (size_t row = 0; row < 8; row++)
    {
        feed_and_execute(&parser, &state, 'x', &redraw, &should_quit);
        if (row + 1 < 8)
        {
            feed_and_execute(&parser, &state, '\r', &redraw, &should_quit);
        }
    }
    assert(state.cursor.row == 7 && state.cursor.column == 1);
    state.is_dirty = false;

    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '5', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '~', &redraw, &should_quit);
    assert(redraw && !should_quit && state.cursor.row == 4);
    assert(state.cursor.column == 1 && !state.is_dirty);

    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '6', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '~', &redraw, &should_quit);
    assert(redraw && !should_quit && state.cursor.row == 7);
    assert(state.cursor.column == 1 && !state.is_dirty);

    editor_state_destroy(&state);
}


static void test_delete_boundaries_through_pipeline(void)
{
    InputParser parser;
    EditorState state;
    bool redraw;
    bool should_quit;
    const DocumentLine *line;

    assert(editor_state_init(&state));
    input_parser_init(&parser);

    /* Delete at the end of the final empty line is a no-op. */
    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '3', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '~', &redraw, &should_quit);
    line = document_get_line(&state.document, 0);
    assert(!redraw && !should_quit && !state.is_dirty);
    assert(state.document.line_count == 1 && line != NULL && line->length == 0);

    /* Enter creates a second empty line; Delete joins it back to the first. */
    feed_and_execute(&parser, &state, '\r', &redraw, &should_quit);
    assert(redraw && state.is_dirty && state.document.line_count == 2);
    state.is_dirty = false;
    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, 'A', &redraw, &should_quit);
    assert(redraw && state.cursor.row == 0 && !state.is_dirty);

    feed_and_execute(&parser, &state, 0x1B, &redraw, &should_quit);
    feed_and_execute(&parser, &state, '[', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '3', &redraw, &should_quit);
    feed_and_execute(&parser, &state, '~', &redraw, &should_quit);
    line = document_get_line(&state.document, 0);
    assert(redraw && !should_quit && state.is_dirty);
    assert(state.document.line_count == 1 && line != NULL && line->length == 0);
    assert(state.cursor.row == 0 && state.cursor.column == 0);

    editor_state_destroy(&state);
}

static void test_utf8_input_and_character_boundary_editing(void)
{
    InputParser parser;
    EditorState state;
    bool redraw;
    bool should_quit;
    const DocumentLine *line;

    assert(editor_state_init(&state));
    input_parser_init(&parser);

    /* U+1F600 arrives as four bytes but becomes one document code point. */
    feed_and_execute(&parser, &state, 0xF0, &redraw, &should_quit);
    assert(!redraw && state.cursor.column == 0);
    feed_and_execute(&parser, &state, 0x9F, &redraw, &should_quit);
    assert(!redraw && state.cursor.column == 0);
    feed_and_execute(&parser, &state, 0x98, &redraw, &should_quit);
    assert(!redraw && state.cursor.column == 0);
    feed_and_execute(&parser, &state, 0x80, &redraw, &should_quit);
    line = document_get_line(&state.document, 0);
    assert(redraw && state.is_dirty && line != NULL && line->length == 1);
    assert(line->data[0] == 0x1F600 && state.cursor.column == 1);

    feed_and_execute(&parser, &state, 'x', &redraw, &should_quit);
    assert(state.cursor.column == 2);
    feed_and_execute(&parser, &state, 0x7F, &redraw, &should_quit);
    line = document_get_line(&state.document, 0);
    assert(redraw && line != NULL && line->length == 1);
    assert(line->data[0] == 0x1F600 && state.cursor.column == 1);

    feed_and_execute(&parser, &state, 0x7F, &redraw, &should_quit);
    line = document_get_line(&state.document, 0);
    assert(redraw && line != NULL && line->length == 0);
    assert(state.cursor.column == 0);

    editor_state_destroy(&state);
}

int main(void)
{
    test_end_to_end_input_pipeline();
    test_page_navigation_pipeline();
    test_alternate_home_end_sequences_reach_commands();
    test_filename_entry_ignores_document_only_keys();
    test_tab_insertion_pipeline();
    test_delete_boundaries_through_pipeline();
    test_utf8_input_and_character_boundary_editing();
    return 0;
}
