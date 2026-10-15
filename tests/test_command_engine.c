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
    EditorCommand command = {type, character};
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

static void test_invalid_arguments(void)
{
    EditorState state;
    EditorCommand command = {EDITOR_COMMAND_NONE, 0};
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

int main(void)
{
    test_editing_commands();
    test_quit_and_noop();
    test_invalid_arguments();
    test_save_as_success_and_backspace();
    test_save_as_failure_and_cancel_preserve_document();
    return 0;
}