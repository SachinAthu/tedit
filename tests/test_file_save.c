#define _POSIX_C_SOURCE 200809L

#include "document/document.h"
#include "file/file.h"
#include "state/editor_state.h"

#include <assert.h>
#include <dirent.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void write_all(int descriptor, const unsigned char *data, size_t length)
{
    size_t offset = 0;

    while (offset < length)
    {
        ssize_t written = write(descriptor, data + offset, length - offset);
        assert(written > 0);
        offset += (size_t)written;
    }
}

static size_t read_file(const char *path, unsigned char *buffer, size_t capacity)
{
    FILE *file = fopen(path, "rb");
    size_t length;

    assert(file != NULL);
    length = fread(buffer, 1, capacity, file);
    assert(!ferror(file));
    assert(feof(file) || length == capacity);
    assert(fclose(file) == 0);
    return length;
}

static void test_editor_state_save_clears_dirty_flag_after_success(void)
{
    char path[] = "/tmp/tedit-save-state-XXXXXX";
    int descriptor = mkstemp(path);
    const unsigned char expected[] = {'A', 0xE2, 0x82, 0xAC, '\n', 'B', '\n'};
    unsigned char actual[32];
    EditorState state;

    assert(descriptor >= 0);
    assert(close(descriptor) == 0);
    assert(editor_state_init(&state));
    assert(document_insert_character(&state.document, 0, 0, 'A'));
    assert(document_insert_character(&state.document, 0, 1, UINT32_C(0x20AC)));
    assert(document_split_line(&state.document, 0, 2));
    assert(document_insert_character(&state.document, 1, 0, 'B'));
    state.is_dirty = true;

    assert(editor_state_save_file(&state, path));
    assert(!state.is_dirty);
    assert(state.document.line_count == 2);
    assert(document_get_line(&state.document, 0)->length == 2);
    assert(document_get_line(&state.document, 0)->data[1] == UINT32_C(0x20AC));
    assert(read_file(path, actual, sizeof(actual)) == sizeof(expected));
    assert(memcmp(actual, expected, sizeof(expected)) == 0);

    editor_state_destroy(&state);
    assert(unlink(path) == 0);
}

static void test_failed_save_preserves_dirty_flag_document_and_destination(void)
{
    char path[] = "/tmp/tedit-save-failure-XXXXXX";
    int descriptor = mkstemp(path);
    const unsigned char original[] = {'o', 'l', 'd'};
    unsigned char actual[16];
    EditorState state;

    assert(descriptor >= 0);
    write_all(descriptor, original, sizeof(original));
    assert(close(descriptor) == 0);

    assert(editor_state_init(&state));
    assert(document_insert_character(&state.document, 0, 0, 'x'));
    state.is_dirty = true;
    /* Simulate corrupted internal state to exercise the serializer's guard. */
    document_get_line(&state.document, 0)->data[0] = UINT32_C(0xD800);

    assert(!editor_state_save_file(&state, path));
    assert(state.is_dirty);
    assert(state.document.line_count == 1);
    assert(document_get_line(&state.document, 0)->length == 1);
    assert(document_get_line(&state.document, 0)->data[0] == UINT32_C(0xD800));
    assert(read_file(path, actual, sizeof(actual)) == sizeof(original));
    assert(memcmp(actual, original, sizeof(original)) == 0);

    editor_state_destroy(&state);
    assert(unlink(path) == 0);
}

static void test_invalid_save_arguments_preserve_dirty_flag(void)
{
    EditorState state;

    assert(editor_state_init(&state));
    state.is_dirty = true;
    assert(!editor_state_save_file(&state, NULL));
    assert(!editor_state_save_file(NULL, "/tmp/tedit-save-invalid"));
    assert(state.is_dirty);
    editor_state_destroy(&state);
}

static void test_replace_failure_preserves_state_and_cleans_temporary_file(void)
{
    char path[] = "/tmp/tedit-save-replace-XXXXXX";
    int descriptor = mkstemp(path);
    const char *base_name;
    char temporary_prefix[128];
    DIR *directory;
    struct dirent *entry;
    struct stat status;
    EditorState state;
    bool temporary_file_found = false;

    assert(descriptor >= 0);
    assert(close(descriptor) == 0);
    assert(unlink(path) == 0);
    assert(mkdir(path, 0700) == 0);

    assert(editor_state_init(&state));
    assert(document_insert_character(&state.document, 0, 0, 'x'));
    state.is_dirty = true;

    /* Renaming a temporary regular file over a directory must fail. */
    assert(!editor_state_save_file(&state, path));
    assert(state.is_dirty);
    assert(state.document.line_count == 1);
    assert(document_get_line(&state.document, 0)->length == 1);
    assert(document_get_line(&state.document, 0)->data[0] == 'x');
    assert(stat(path, &status) == 0);
    assert(S_ISDIR(status.st_mode));

    base_name = strrchr(path, '/');
    assert(base_name != NULL);
    ++base_name;
    assert(snprintf(temporary_prefix, sizeof(temporary_prefix), "%s.tedit-", base_name) > 0);

    directory = opendir("/tmp");
    assert(directory != NULL);
    while ((entry = readdir(directory)) != NULL)
    {
        if (strncmp(entry->d_name, temporary_prefix, strlen(temporary_prefix)) == 0)
        {
            temporary_file_found = true;
            break;
        }
    }
    assert(closedir(directory) == 0);
    assert(!temporary_file_found);

    editor_state_destroy(&state);
    assert(rmdir(path) == 0);
}

static void test_zero_line_document_saves_empty_file(void)
{
    char path[] = "/tmp/tedit-save-empty-XXXXXX";
    int descriptor = mkstemp(path);
    unsigned char actual[4];
    Document document;

    assert(descriptor >= 0);
    assert(close(descriptor) == 0);
    assert(document_init(&document));
    assert(file_load_document(path, &document) == FILE_LOAD_OK);
    assert(document.line_count == 0);

    assert(file_save_document(path, &document) == FILE_SAVE_OK);
    assert(read_file(path, actual, sizeof(actual)) == 0);

    assert(unlink(path) == 0);
}

int main(void)
{
    test_editor_state_save_clears_dirty_flag_after_success();
    test_failed_save_preserves_dirty_flag_document_and_destination();
    test_invalid_save_arguments_preserve_dirty_flag();
    test_replace_failure_preserves_state_and_cleans_temporary_file();
    test_zero_line_document_saves_empty_file();
    puts("File save tests passed.");
    return 0;
}
