#define _POSIX_C_SOURCE 200809L
#include "command/command_engine.h"
#include "state/editor_state.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void execute(EditorState *state, EditorCommandType type, uint32_t character,
                    bool *redraw, bool *should_quit)
{
    EditorCommand command = {type, character, 3};
    assert(command_engine_execute(state, &command, NULL, redraw, should_quit));
}

static void test_editing_commands(void)
{
    EditorState state;
    bool redraw;
    bool should_quit;
    const DocumentLine *line;

    assert(editor_state_init(&state));
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'a', &redraw, &should_quit);
    assert(redraw && !should_quit && state.is_dirty);
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'b', &redraw, &should_quit);
    assert(redraw);
    execute(&state, EDITOR_COMMAND_MOVE_LEFT, 0, &redraw, &should_quit);
    assert(redraw && state.cursor.column == 1);
    execute(&state, EDITOR_COMMAND_BACKSPACE, 0, &redraw, &should_quit);
    assert(redraw && state.cursor.column == 0);
    line = document_get_line(&state.document, 0);
    assert(line != NULL && line->length == 1 && line->data[0] == 'b');
    execute(&state, EDITOR_COMMAND_INSERT_NEWLINE, 0, &redraw, &should_quit);
    assert(redraw && state.document.line_count == 2 && state.cursor.row == 1);
    editor_state_destroy(&state);
}

static void test_tab_insertion_uses_document_code_point(void)
{
    EditorState state;
    bool redraw;
    bool should_quit;
    const DocumentLine *line;

    assert(editor_state_init(&state));
    assert(state.configuration.tab_width == DEFAULT_TAB_WIDTH);
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, (uint32_t)'a', &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_INSERT_TAB, 0, &redraw, &should_quit);

    line = document_get_line(&state.document, 0);
    assert(redraw && !should_quit && state.is_dirty);
    assert(line != NULL && line->length == 2);
    assert(line->data[0] == (uint32_t)'a' && line->data[1] == (uint32_t)'\t');
    assert(state.cursor.column == 2);

    editor_state_destroy(&state);
}

static void test_home_and_end_commands(void)
{
    EditorState state;
    bool redraw;
    bool should_quit;

    assert(editor_state_init(&state));
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'a', &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'b', &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'c', &redraw, &should_quit);
    assert(state.cursor.column == 3);

    execute(&state, EDITOR_COMMAND_MOVE_HOME, 0, &redraw, &should_quit);
    assert(redraw && !should_quit && state.cursor.row == 0 && state.cursor.column == 0);
    execute(&state, EDITOR_COMMAND_MOVE_HOME, 0, &redraw, &should_quit);
    assert(!redraw && state.cursor.column == 0);

    execute(&state, EDITOR_COMMAND_MOVE_END, 0, &redraw, &should_quit);
    assert(redraw && !should_quit && state.cursor.column == 3);
    execute(&state, EDITOR_COMMAND_MOVE_END, 0, &redraw, &should_quit);
    assert(!redraw && state.cursor.column == 3);
    assert(state.is_dirty);

    editor_state_destroy(&state);
}


static void test_page_navigation_commands(void)
{
    EditorState state;
    bool redraw;
    bool should_quit;

    assert(editor_state_init(&state));

    /* Build eight logical lines, each with four code points. */
    for (size_t row = 0; row < 8; row++)
    {
        for (size_t column = 0; column < 4; column++)
        {
            execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, (uint32_t)('a' + column),
                    &redraw, &should_quit);
        }
        if (row + 1 < 8)
        {
            execute(&state, EDITOR_COMMAND_INSERT_NEWLINE, 0, &redraw, &should_quit);
        }
    }

    assert(state.cursor.row == 7 && state.cursor.column == 4);

    /* Make one destination line shorter to verify preferred-column recovery. */
    assert(document_delete_character(&state.document, 4, 1));
    assert(document_delete_character(&state.document, 4, 1));
    assert(document_delete_character(&state.document, 4, 1));
    state.is_dirty = false;

    execute(&state, EDITOR_COMMAND_PAGE_UP, 0, &redraw, &should_quit);
    assert(redraw && !should_quit);
    assert(state.cursor.row == 4 && state.cursor.column == 1);
    assert(state.cursor.preferred_column == 4);
    assert(!state.is_dirty);

    execute(&state, EDITOR_COMMAND_PAGE_UP, 0, &redraw, &should_quit);
    assert(redraw && state.cursor.row == 1 && state.cursor.column == 4);
    execute(&state, EDITOR_COMMAND_PAGE_UP, 0, &redraw, &should_quit);
    assert(redraw && state.cursor.row == 0 && state.cursor.column == 4);
    execute(&state, EDITOR_COMMAND_PAGE_UP, 0, &redraw, &should_quit);
    assert(!redraw && state.cursor.row == 0);

    execute(&state, EDITOR_COMMAND_PAGE_DOWN, 0, &redraw, &should_quit);
    assert(redraw && state.cursor.row == 3 && state.cursor.column == 4);
    execute(&state, EDITOR_COMMAND_PAGE_DOWN, 0, &redraw, &should_quit);
    assert(redraw && state.cursor.row == 6 && state.cursor.column == 4);
    execute(&state, EDITOR_COMMAND_PAGE_DOWN, 0, &redraw, &should_quit);
    assert(redraw && state.cursor.row == 7 && state.cursor.column == 4);
    execute(&state, EDITOR_COMMAND_PAGE_DOWN, 0, &redraw, &should_quit);
    assert(!redraw && state.cursor.row == 7);
    assert(!state.is_dirty);

    editor_state_destroy(&state);
}

static void test_document_delete_command(void)
{
    EditorState state;
    bool redraw;
    bool should_quit;
    const DocumentLine *line;

    assert(editor_state_init(&state));
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'a', &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 0x20AC, &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'c', &redraw, &should_quit);
    assert(state.cursor.column == 3 && state.is_dirty);

    state.is_dirty = false;
    execute(&state, EDITOR_COMMAND_MOVE_HOME, 0, &redraw, &should_quit);
    assert(redraw && !state.is_dirty);
    execute(&state, EDITOR_COMMAND_DELETE_CHARACTER, 0, &redraw, &should_quit);
    line = document_get_line(&state.document, 0);
    assert(redraw && !should_quit && state.is_dirty);
    assert(line != NULL && line->length == 2);
    assert(line->data[0] == 0x20AC && line->data[1] == 'c');
    assert(state.cursor.row == 0 && state.cursor.column == 0);

    state.is_dirty = false;
    execute(&state, EDITOR_COMMAND_MOVE_END, 0, &redraw, &should_quit);
    assert(redraw && !state.is_dirty);
    execute(&state, EDITOR_COMMAND_INSERT_NEWLINE, 0, &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'z', &redraw, &should_quit);
    state.is_dirty = false;
    execute(&state, EDITOR_COMMAND_MOVE_HOME, 0, &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_MOVE_UP, 0, &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_MOVE_END, 0, &redraw, &should_quit);
    assert(state.cursor.row == 0 && state.cursor.column == 2);

    execute(&state, EDITOR_COMMAND_DELETE_CHARACTER, 0, &redraw, &should_quit);
    assert(redraw && state.is_dirty && state.document.line_count == 1);
    line = document_get_line(&state.document, 0);
    assert(line != NULL && line->length == 3);
    assert(line->data[0] == 0x20AC && line->data[1] == 'c' && line->data[2] == 'z');
    assert(state.cursor.row == 0 && state.cursor.column == 2);

    state.is_dirty = false;
    execute(&state, EDITOR_COMMAND_MOVE_END, 0, &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_DELETE_CHARACTER, 0, &redraw, &should_quit);
    assert(!redraw && !state.is_dirty && state.document.line_count == 1);

    editor_state_destroy(&state);
}

static void test_backspace_deletes_one_unicode_code_point(void)
{
    EditorState state;
    bool redraw;
    bool should_quit;
    const DocumentLine *line;

    assert(editor_state_init(&state));
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 0x1F600, &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, (uint32_t)'x', &redraw, &should_quit);
    line = document_get_line(&state.document, 0);
    assert(line != NULL && line->length == 2);
    assert(line->data[0] == 0x1F600 && line->data[1] == (uint32_t)'x');
    assert(state.cursor.column == 2);

    state.is_dirty = false;
    execute(&state, EDITOR_COMMAND_BACKSPACE, 0, &redraw, &should_quit);
    line = document_get_line(&state.document, 0);
    assert(redraw && state.is_dirty && line != NULL && line->length == 1);
    assert(line->data[0] == 0x1F600 && state.cursor.column == 1);

    state.is_dirty = false;
    execute(&state, EDITOR_COMMAND_BACKSPACE, 0, &redraw, &should_quit);
    line = document_get_line(&state.document, 0);
    assert(redraw && state.is_dirty && line != NULL && line->length == 0);
    assert(state.cursor.column == 0);

    editor_state_destroy(&state);
}

static void test_quit_and_noop(void)
{
    EditorState state;
    bool redraw;
    bool should_quit;

    assert(editor_state_init(&state));
    execute(&state, EDITOR_COMMAND_NONE, 0, &redraw, &should_quit);
    assert(!redraw && !should_quit);
    execute(&state, EDITOR_COMMAND_QUIT, 0, &redraw, &should_quit);
    assert(!redraw && should_quit);
    editor_state_destroy(&state);
}


static void test_filename_entry_cursor_editing(void)
{
    EditorState state;
    bool redraw;
    bool should_quit;

    assert(editor_state_init(&state));
    execute(&state, EDITOR_COMMAND_SAVE, 0, &redraw, &should_quit);
    assert(state.filename_entry_active && redraw);

    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'a', &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'b', &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'c', &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_MOVE_LEFT, 0, &redraw, &should_quit);
    assert(state.filename_entry_cursor == 2);

    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'X', &redraw, &should_quit);
    assert(strcmp(state.filename_entry, "abXc") == 0);
    assert(state.filename_entry_cursor == 3);
    execute(&state, EDITOR_COMMAND_MOVE_LEFT, 0, &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_DELETE_CHARACTER, 0, &redraw, &should_quit);
    assert(strcmp(state.filename_entry, "abc") == 0);
    assert(state.filename_entry_cursor == 2);
    execute(&state, EDITOR_COMMAND_DELETE_CHARACTER, 0, &redraw, &should_quit);
    assert(strcmp(state.filename_entry, "ab") == 0);

    execute(&state, EDITOR_COMMAND_MOVE_LEFT, 0, &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_BACKSPACE, 0, &redraw, &should_quit);
    assert(strcmp(state.filename_entry, "b") == 0);
    assert(state.filename_entry_cursor == 0);

    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 0x20AC, &redraw, &should_quit);
    assert(state.filename_entry_length == 4);
    assert(state.filename_entry_cursor == 3);
    execute(&state, EDITOR_COMMAND_MOVE_LEFT, 0, &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_DELETE_CHARACTER, 0, &redraw, &should_quit);
    assert(strcmp(state.filename_entry, "b") == 0);
    assert(state.filename_entry_length == 1 && state.filename_entry_cursor == 0);

    execute(&state, EDITOR_COMMAND_CANCEL, 0, &redraw, &should_quit);
    assert(!state.filename_entry_active && !state.is_dirty && state.file_path == NULL);
    editor_state_destroy(&state);
}

static void test_dirty_quit_confirmation(void)
{
    EditorState state;
    bool redraw;
    bool should_quit;

    assert(editor_state_init(&state));
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'q', &redraw, &should_quit);
    assert(state.is_dirty);

    execute(&state, EDITOR_COMMAND_QUIT, 0, &redraw, &should_quit);
    assert(redraw && !should_quit && state.quit_confirmation_active);
    assert(strstr(state.message, "Save changes before quitting?") != NULL);

    execute(&state, EDITOR_COMMAND_CANCEL, 0, &redraw, &should_quit);
    assert(redraw && !should_quit && !state.quit_confirmation_active);
    assert(state.is_dirty);
    assert(strcmp(state.message, "Quit cancelled.") == 0);

    execute(&state, EDITOR_COMMAND_QUIT, 0, &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'n', &redraw, &should_quit);
    assert(!redraw && should_quit && !state.quit_confirmation_active);
    assert(state.is_dirty);
    editor_state_destroy(&state);
}

static void test_dirty_quit_saves_unnamed_document(void)
{
    char path[] = "/tmp/tedit-quit-save-XXXXXX";
    int descriptor = mkstemp(path);
    EditorState state;
    bool redraw;
    bool should_quit;
    FILE *file;
    char contents[16];
    size_t length;

    assert(descriptor >= 0);
    assert(close(descriptor) == 0);
    assert(unlink(path) == 0);
    assert(editor_state_init(&state));
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'z', &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_QUIT, 0, &redraw, &should_quit);
    assert(state.quit_confirmation_active && !should_quit);

    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'y', &redraw, &should_quit);
    assert(state.filename_entry_active && state.quit_after_save && !should_quit);
    for (size_t index = 0; path[index] != '\0'; ++index)
    {
        execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, (unsigned char)path[index],
                &redraw, &should_quit);
    }
    execute(&state, EDITOR_COMMAND_INSERT_NEWLINE, 0, &redraw, &should_quit);
    assert(redraw && should_quit && !state.filename_entry_active);
    assert(state.file_path != NULL && strcmp(state.file_path, path) == 0);
    assert(!state.is_dirty && !state.quit_after_save);

    file = fopen(path, "rb");
    assert(file != NULL);
    length = fread(contents, 1, sizeof(contents), file);
    assert(!ferror(file));
    assert(fclose(file) == 0);
    assert(length == 2 && contents[0] == 'z' && contents[1] == '\n');

    editor_state_destroy(&state);
    assert(unlink(path) == 0);
}

static void test_invalid_arguments(void)
{
    EditorState state;
    EditorCommand command = {EDITOR_COMMAND_NONE, 0, 0};
    bool redraw;
    bool should_quit;

    assert(editor_state_init(&state));
    assert(!command_engine_execute(NULL, &command, NULL, &redraw, &should_quit));
    assert(!command_engine_execute(&state, NULL, NULL, &redraw, &should_quit));
    assert(!command_engine_execute(&state, &command, NULL, NULL, &should_quit));
    assert(!command_engine_execute(&state, &command, NULL, &redraw, NULL));
    editor_state_destroy(&state);
}


static void test_save_as_success_and_backspace(void)
{
    char path[] = "/tmp/tedit-save-as-XXXXXX";
    int descriptor = mkstemp(path);
    EditorState state;
    bool redraw;
    bool should_quit;
    unsigned char saved[8];
    FILE *file;
    size_t length;

    assert(descriptor >= 0);
    assert(close(descriptor) == 0);
    assert(unlink(path) == 0);
    assert(editor_state_init(&state));
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'x', &redraw, &should_quit);
    assert(state.is_dirty);
    execute(&state, EDITOR_COMMAND_SAVE, 0, &redraw, &should_quit);
    assert(redraw && state.filename_entry_active && state.is_dirty);

    for (size_t index = 0; path[index] != '\0'; ++index)
    {
        execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, (unsigned char)path[index],
                &redraw, &should_quit);
        assert(redraw && state.filename_entry_active);
    }
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'x', &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_BACKSPACE, 0, &redraw, &should_quit);
    assert(strcmp(state.filename_entry, path) == 0);
    execute(&state, EDITOR_COMMAND_INSERT_NEWLINE, 0, &redraw, &should_quit);

    assert(redraw && !state.filename_entry_active);
    assert(state.file_path != NULL && strcmp(state.file_path, path) == 0);
    assert(!state.is_dirty);
    assert(strcmp(state.message, "Saved successfully.") == 0);

    file = fopen(path, "rb");
    assert(file != NULL);
    length = fread(saved, 1, sizeof(saved), file);
    assert(!ferror(file));
    assert(fclose(file) == 0);
    assert(length == 2 && saved[0] == 'x' && saved[1] == '\n');

    editor_state_destroy(&state);
    assert(unlink(path) == 0);
}

static void test_save_as_failure_and_cancel_preserve_document(void)
{
    EditorState state;
    bool redraw;
    bool should_quit;
    const DocumentLine *line;
    char invalid_path[] = "/tmp/tedit-save-as-fail-XXXXXX";
    int descriptor = mkstemp(invalid_path);

    assert(descriptor >= 0);
    assert(close(descriptor) == 0);
    assert(unlink(invalid_path) == 0);
    assert(mkdir(invalid_path, 0700) == 0);
    assert(editor_state_init(&state));
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'q', &redraw, &should_quit);
    execute(&state, EDITOR_COMMAND_SAVE, 0, &redraw, &should_quit);
    assert(state.filename_entry_active && state.is_dirty);

    for (size_t index = 0; invalid_path[index] != '\0'; ++index)
    {
        execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, (unsigned char)invalid_path[index],
                &redraw, &should_quit);
    }
    execute(&state, EDITOR_COMMAND_INSERT_NEWLINE, 0, &redraw, &should_quit);

    assert(redraw && state.filename_entry_active);
    assert(state.file_path == NULL);
    assert(state.is_dirty);
    line = document_get_line(&state.document, 0);
    assert(line != NULL && line->length == 1 && line->data[0] == 'q');
    assert(strstr(state.message, "Save failed") != NULL);

    execute(&state, EDITOR_COMMAND_CANCEL, 0, &redraw, &should_quit);
    assert(redraw && !state.filename_entry_active);
    assert(state.file_path == NULL && state.is_dirty);
    line = document_get_line(&state.document, 0);
    assert(line != NULL && line->length == 1 && line->data[0] == 'q');
    assert(strcmp(state.message, "Save As cancelled.") == 0);
    editor_state_destroy(&state);
    assert(rmdir(invalid_path) == 0);
}


static void test_dirty_quit_saves_named_document(void)
{
    char path[] = "/tmp/tedit-quit-named-XXXXXX";
    int descriptor = mkstemp(path);
    EditorState state;
    bool redraw;
    bool should_quit;
    FILE *file;
    char contents[16];
    size_t length;

    assert(descriptor >= 0);
    assert(close(descriptor) == 0);
    assert(unlink(path) == 0);
    assert(editor_state_init(&state));
    assert(command_engine_save_as(&state, path));
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'n', &redraw, &should_quit);
    assert(state.is_dirty);

    execute(&state, EDITOR_COMMAND_QUIT, 0, &redraw, &should_quit);
    assert(redraw && !should_quit && state.quit_confirmation_active);
    execute(&state, EDITOR_COMMAND_INSERT_CHARACTER, 'Y', &redraw, &should_quit);
    assert(redraw && should_quit && !state.quit_confirmation_active);
    assert(!state.is_dirty && state.file_path != NULL && strcmp(state.file_path, path) == 0);

    file = fopen(path, "rb");
    assert(file != NULL);
    length = fread(contents, 1, sizeof(contents), file);
    assert(!ferror(file));
    assert(fclose(file) == 0);
    assert(length == 2 && contents[0] == 'n' && contents[1] == '\n');

    editor_state_destroy(&state);
    assert(unlink(path) == 0);
}

int main(void)
{
    test_editing_commands();
    test_tab_insertion_uses_document_code_point();
    test_home_and_end_commands();
    test_page_navigation_commands();
    test_document_delete_command();
    test_backspace_deletes_one_unicode_code_point();
    test_quit_and_noop();
    test_invalid_arguments();
    test_save_as_success_and_backspace();
    test_save_as_failure_and_cancel_preserve_document();
    test_filename_entry_cursor_editing();
    test_dirty_quit_confirmation();
    test_dirty_quit_saves_unnamed_document();
    test_dirty_quit_saves_named_document();
    return 0;
}
