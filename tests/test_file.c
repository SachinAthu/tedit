
#define _POSIX_C_SOURCE 200809L

#include "document/document.h"
#include "file/file.h"
#include "state/editor_state.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void write_all(int descriptor, const unsigned char *data, size_t length)
{
    size_t offset = 0;

    while (offset < length)
    {
        ssize_t written = write(
            descriptor,
            data + offset,
            length - offset);

        assert(written > 0);
        offset += (size_t)written;
    }
}

static void test_load_utf8_and_line_endings(void)
{
    char path[] = "/tmp/tedit-file-XXXXXX";
    int descriptor = mkstemp(path);

    /* A + U+20AC EURO SIGN + CRLF + B + LF */
    const unsigned char content[] = {
        0x41,
        0xE2, 0x82, 0xAC,
        0x0D, 0x0A,
        0x42,
        0x0A};

    Document document;

    assert(descriptor >= 0);

    write_all(descriptor, content, sizeof(content));
    assert(close(descriptor) == 0);

    assert(document_init(&document));

    assert(file_load_document(path, &document) == FILE_LOAD_OK);

    assert(document.line_count == 2);

    DocumentLine *first = document_get_line(&document, 0);
    DocumentLine *second = document_get_line(&document, 1);

    assert(first != NULL);
    assert(second != NULL);

    assert(first->length == 2);
    assert(first->data[0] == UINT32_C(0x41));
    assert(first->data[1] == UINT32_C(0x20AC));

    assert(second->length == 1);
    assert(second->data[0] == UINT32_C(0x42));

    document_destroy(&document);
    assert(unlink(path) == 0);
}

static void test_empty_file_has_zero_lines(void)
{
    char path[] = "/tmp/tedit-file-XXXXXX";
    int descriptor = mkstemp(path);
    Document document;

    assert(descriptor >= 0);
    assert(close(descriptor) == 0);

    assert(document_init(&document));
    assert(file_load_document(path, &document) == FILE_LOAD_OK);
    assert(document.line_count == 0);

    document_destroy(&document);
    assert(unlink(path) == 0);
}

static void test_invalid_utf8_preserves_destination(void)
{
    char path[] = "/tmp/tedit-file-XXXXXX";
    int descriptor = mkstemp(path);

    /* C0 AF is an invalid overlong UTF-8 encoding. */
    const unsigned char invalid[] = {0xC0, 0xAF};

    Document document;

    assert(descriptor >= 0);

    write_all(descriptor, invalid, sizeof(invalid));
    assert(close(descriptor) == 0);

    assert(document_init(&document));
    assert(document_insert_character(&document, 0, 0, 'x'));

    assert(file_load_document(path, &document) ==
           FILE_LOAD_INVALID_UTF8);

    assert(document.line_count == 1);

    DocumentLine *line = document_get_line(&document, 0);

    assert(line != NULL);
    assert(line->length == 1);
    assert(line->data[0] == 'x');

    document_destroy(&document);
    assert(unlink(path) == 0);
}

static void test_missing_file_preserves_destination(void)
{
    Document document;

    assert(document_init(&document));
    assert(document_insert_character(&document, 0, 0, 'z'));

    assert(file_load_document(
               "/tmp/tedit-file-that-does-not-exist",
               &document) == FILE_LOAD_OPEN_FAILED);

    assert(document.line_count == 1);

    DocumentLine *line = document_get_line(&document, 0);

    assert(line != NULL);
    assert(line->length == 1);
    assert(line->data[0] == 'z');

    document_destroy(&document);
}

static void test_editor_state_load_resets_cursor_viewport_and_dirty_flag(void)
{
    char path[] = "/tmp/tedit-state-load-XXXXXX";
    int descriptor = mkstemp(path);
    const unsigned char content[] = {'a', '\n', 'b', 'c'};
    EditorState state;

    assert(descriptor >= 0);
    write_all(descriptor, content, sizeof(content));
    assert(close(descriptor) == 0);

    assert(editor_state_init(&state));
    state.cursor.row = 12;
    state.cursor.column = 7;
    state.cursor.preferred_column = 7;
    state.viewport.viewport_row = 5;
    state.viewport.viewport_column = 9;
    state.is_dirty = true;

    assert(editor_state_load_file(&state, path));
    assert(state.document.line_count == 2);
    assert(state.cursor.row == 0);
    assert(state.cursor.column == 0);
    assert(state.cursor.preferred_column == 0);
    assert(state.viewport.viewport_row == 0);
    assert(state.viewport.viewport_column == 0);
    assert(!state.is_dirty);
    assert(state.file_path != NULL);
    assert(strcmp(state.file_path, path) == 0);

    assert(document_get_line(&state.document, 0)->length == 1);
    assert(document_get_line(&state.document, 0)->data[0] == (uint32_t)'a');
    assert(document_get_line(&state.document, 1)->length == 2);
    assert(document_get_line(&state.document, 1)->data[0] == (uint32_t)'b');
    assert(document_get_line(&state.document, 1)->data[1] == (uint32_t)'c');

    editor_state_destroy(&state);
    assert(unlink(path) == 0);
}

static void test_editor_state_failed_load_preserves_state(void)
{
    EditorState state;

    assert(editor_state_init(&state));
    assert(document_insert_character(&state.document, 0, 0, 'x'));
    state.cursor.row = 0;
    state.cursor.column = 1;
    state.cursor.preferred_column = 1;
    state.viewport.viewport_row = 3;
    state.viewport.viewport_column = 4;
    state.is_dirty = true;
    state.file_path = malloc(sizeof("original.txt"));
    assert(state.file_path != NULL);
    memcpy(state.file_path, "original.txt", sizeof("original.txt"));

    assert(!editor_state_load_file(&state, "/tmp/tedit-no-such-file"));

    assert(state.document.line_count == 1);
    assert(document_get_line(&state.document, 0)->length == 1);
    assert(document_get_line(&state.document, 0)->data[0] == (uint32_t)'x');
    assert(state.cursor.row == 0 && state.cursor.column == 1);
    assert(state.cursor.preferred_column == 1);
    assert(state.viewport.viewport_row == 3);
    assert(state.viewport.viewport_column == 4);
    assert(state.is_dirty);
    assert(state.file_path != NULL);
    assert(strcmp(state.file_path, "original.txt") == 0);

    editor_state_destroy(&state);
}

int main(void)
{
    test_load_utf8_and_line_endings();
    test_empty_file_has_zero_lines();
    test_invalid_utf8_preserves_destination();
    test_missing_file_preserves_destination();
    test_editor_state_load_resets_cursor_viewport_and_dirty_flag();
    test_editor_state_failed_load_preserves_state();

    puts("File Manager tests passed.");

    return 0;
}
