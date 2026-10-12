#include "document/document.h"
#include "cursor/cursor.h"
#include "state/editor_state.h"
#include "viewport.h"

#include <assert.h>
#include <string.h>

static void test_document_insert_and_delete(void)
{
    Document document;
    DocumentLine *line;

    assert(document_init(&document));
    assert(document_insert_character(&document, 0, 0, 'a'));
    assert(document_insert_character(&document, 0, 1, 'c'));
    assert(document_insert_character(&document, 0, 1, 'b'));

    line = document_get_line(&document, 0);
    assert(line != NULL);
    assert(line->length == 3);
    assert(memcmp(line->data, "abc", 3) == 0);

    assert(document_delete_character(&document, 0, 1));
    assert(line->length == 2);
    assert(memcmp(line->data, "ac", 2) == 0);

    assert(!document_delete_character(&document, 0, 2));
    assert(!document_delete_character(&document, 1, 0));
    assert(line->length == 2);
    assert(memcmp(line->data, "ac", 2) == 0);

    document_destroy(&document);
}

static void test_cursor_horizontal_movement(void)
{
    Document document;
    Cursor cursor;

    assert(document_init(&document));
    assert(document_insert_character(&document, 0, 0, 'x'));
    assert(document_insert_character(&document, 0, 1, 'y'));
    cursor_init(&cursor);

    assert(!cursor_move_left(&cursor));
    assert(cursor.column == 0);
    assert(cursor_move_right(&cursor, &document));
    assert(cursor.column == 1);
    assert(cursor_move_left(&cursor));
    assert(cursor.column == 0);
    assert(cursor_move_right(&cursor, &document));
    assert(cursor_move_right(&cursor, &document));
    assert(cursor.column == 2);
    assert(!cursor_move_right(&cursor, &document));
    assert(cursor.column == 2);

    document_destroy(&document);
}

static void test_cursor_vertical_movement_preserves_preferred_column(void)
{
    Document document;
    Cursor cursor;

    assert(document_init(&document));
    assert(document_insert_character(&document, 0, 0, 'a'));
    assert(document_insert_character(&document, 0, 1, 'b'));
    assert(document_insert_character(&document, 0, 2, 'c'));
    assert(document_insert_character(&document, 0, 3, 'd'));
    assert(document_insert_character(&document, 0, 4, 'e'));
    assert(document_insert_character(&document, 0, 5, 'f'));
    assert(document_split_line(&document, 0, 6));
    assert(document_insert_character(&document, 1, 0, 'x'));
    assert(document_insert_character(&document, 1, 1, 'y'));
    assert(document_split_line(&document, 1, 2));
    assert(document_split_line(&document, 2, 0));
    assert(document_insert_character(&document, 3, 0, '1'));
    assert(document_insert_character(&document, 3, 1, '2'));
    assert(document_insert_character(&document, 3, 2, '3'));
    assert(document_insert_character(&document, 3, 3, '4'));
    assert(document_insert_character(&document, 3, 4, '5'));
    assert(document_insert_character(&document, 3, 5, '6'));

    cursor_init(&cursor);
    assert(cursor_set_position(&cursor, &document, 0, 5));
    assert(cursor_move_down(&cursor, &document));
    assert(cursor.row == 1 && cursor.column == 2);
    assert(cursor.preferred_column == 5);
    assert(cursor_move_down(&cursor, &document));
    assert(cursor.row == 2 && cursor.column == 0);
    assert(cursor.preferred_column == 5);
    assert(cursor_move_down(&cursor, &document));
    assert(cursor.row == 3 && cursor.column == 5);
    assert(cursor.preferred_column == 5);
    assert(cursor_move_up(&cursor, &document));
    assert(cursor.row == 2 && cursor.column == 0);
    assert(cursor_move_up(&cursor, &document));
    assert(cursor.row == 1 && cursor.column == 2);
    assert(cursor_move_up(&cursor, &document));
    assert(cursor.row == 0 && cursor.column == 5);
    assert(!cursor_move_up(&cursor, &document));
    assert(!cursor_move_down(NULL, &document));

    document_destroy(&document);
}

static void test_horizontal_movement_resets_preferred_column(void)
{
    Document document;
    Cursor cursor;

    assert(document_init(&document));
    assert(document_insert_character(&document, 0, 0, 'a'));
    assert(document_insert_character(&document, 0, 1, 'b'));
    assert(document_split_line(&document, 0, 2));
    assert(document_insert_character(&document, 1, 0, 'x'));

    cursor_init(&cursor);
    assert(cursor_set_position(&cursor, &document, 0, 2));
    assert(cursor_move_down(&cursor, &document));
    assert(cursor.column == 1 && cursor.preferred_column == 2);
    assert(cursor_move_left(&cursor));
    assert(cursor.column == 0 && cursor.preferred_column == 0);

    document_destroy(&document);
}

static void test_dirty_flag_initial_state(void)
{
    EditorState state;

    assert(editor_state_init(&state));
    assert(!state.is_dirty);
    state.is_dirty = true;
    assert(state.is_dirty);
    editor_state_destroy(&state);
}

static void test_document_split_line(void)
{
    Document document;
    DocumentLine *first_line;
    DocumentLine *second_line;

    assert(document_init(&document));
    assert(document_insert_character(&document, 0, 0, 'a'));
    assert(document_insert_character(&document, 0, 1, 'b'));
    assert(document_insert_character(&document, 0, 2, 'c'));

    assert(document_split_line(&document, 0, 1));
    assert(document.line_count == 2);
    first_line = document_get_line(&document, 0);
    second_line = document_get_line(&document, 1);
    assert(first_line != NULL && second_line != NULL);
    assert(first_line->length == 1 && memcmp(first_line->data, "a", 1) == 0);
    assert(second_line->length == 2 && memcmp(second_line->data, "bc", 2) == 0);

    assert(document_split_line(&document, 1, 0));
    assert(document.line_count == 3);
    assert(document_get_line(&document, 1)->length == 0);
    assert(document_get_line(&document, 2)->length == 2);

    assert(document_split_line(&document, 2, 2));
    assert(document.line_count == 4);
    assert(document_get_line(&document, 2)->length == 2);
    assert(document_get_line(&document, 3)->length == 0);
    assert(!document_split_line(&document, 4, 0));
    assert(!document_split_line(&document, 0, 2));

    document_destroy(&document);
}

static void test_document_merge_lines(void)
{
    Document document;
    DocumentLine *line;

    assert(document_init(&document));
    assert(document_insert_character(&document, 0, 0, 'a'));
    assert(document_insert_character(&document, 0, 1, 'b'));
    assert(document_split_line(&document, 0, 2));
    assert(document_insert_character(&document, 1, 0, 'c'));
    assert(document_insert_character(&document, 1, 1, 'd'));

    assert(document_merge_lines(&document, 0));
    assert(document.line_count == 1);
    line = document_get_line(&document, 0);
    assert(line != NULL && line->length == 4);
    assert(memcmp(line->data, "abcd", 4) == 0);
    assert(!document_merge_lines(&document, 0));
    assert(!document_merge_lines(&document, 1));

    document_destroy(&document);
}

static void test_line_editing_cursor_transitions(void)
{
    EditorState state;
    const DocumentLine *previous_line;

    assert(editor_state_init(&state));
    assert(document_insert_character(&state.document, 0, 0, 'a'));
    assert(document_insert_character(&state.document, 0, 1, 'b'));
    assert(cursor_set_position(&state.cursor, &state.document, 0, 1));

    assert(document_split_line(&state.document, state.cursor.row, state.cursor.column));
    state.cursor.row += 1;
    state.cursor.column = 0;
    state.is_dirty = true;
    assert(state.cursor.row == 1 && state.cursor.column == 0);
    assert(state.document.line_count == 2);

    assert(document_insert_character(&state.document, 1, 0, 'c'));
    assert(cursor_set_position(&state.cursor, &state.document, 1, 0));
    previous_line = document_get_line(&state.document, 0);
    assert(previous_line != NULL);
    state.cursor.row -= 1;
    state.cursor.column = previous_line->length;
    assert(document_merge_lines(&state.document, state.cursor.row));
    assert(state.cursor.row == 0 && state.cursor.column == 1);
    assert(state.document.line_count == 1);
    assert(state.is_dirty);

    editor_state_destroy(&state);
}

static void test_viewport_tracks_cursor_visibility(void)
{
    Viewport viewport;

    viewport_init(&viewport);
    assert(viewport.viewport_row == 0);
    assert(viewport.viewport_column == 0);

    assert(viewport_ensure_cursor_visible(&viewport, 2, 3, 4, 5));
    assert(viewport.viewport_row == 0);
    assert(viewport.viewport_column == 0);

    assert(viewport_ensure_cursor_visible(&viewport, 4, 6, 4, 5));
    assert(viewport.viewport_row == 1);
    assert(viewport.viewport_column == 2);

    assert(viewport_ensure_cursor_visible(&viewport, 0, 1, 4, 5));
    assert(viewport.viewport_row == 0);
    assert(viewport.viewport_column == 1);
}

static void test_viewport_rejects_invalid_dimensions(void)
{
    Viewport viewport;

    viewport_init(&viewport);
    assert(!viewport_ensure_cursor_visible(&viewport, 0, 0, 0, 5));
    assert(!viewport_ensure_cursor_visible(&viewport, 0, 0, 5, 0));
    assert(!viewport_ensure_cursor_visible(NULL, 0, 0, 5, 5));
}

int main(void)
{
    test_document_insert_and_delete();
    test_cursor_horizontal_movement();
    test_cursor_vertical_movement_preserves_preferred_column();
    test_horizontal_movement_resets_preferred_column();
    test_dirty_flag_initial_state();
    test_document_split_line();
    test_document_merge_lines();
    test_line_editing_cursor_transitions();
    test_viewport_tracks_cursor_visibility();
    test_viewport_rejects_invalid_dimensions();

    return 0;
}