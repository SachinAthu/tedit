#include "document/document.h"
#include "cursor/cursor.h"
#include "state/editor_state.h"

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

static void test_dirty_flag_initial_state(void)
{
    EditorState state;

    assert(editor_state_init(&state));
    assert(!state.is_dirty);
    state.is_dirty = true;
    assert(state.is_dirty);
    editor_state_destroy(&state);
}

int main(void)
{
    test_document_insert_and_delete();
    test_cursor_horizontal_movement();
    test_dirty_flag_initial_state();
    return 0;
}