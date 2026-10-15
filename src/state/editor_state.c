#include "editor_state.h"
#include "file/file.h"

#include <string.h>
#include <stdlib.h>

bool editor_state_init(EditorState *state)
{
    if (state == NULL)
    {
        return false;
    }

    /* Zero every field first so partial failures leave a clean state. */
    memset(state, 0, sizeof(*state));

    if (!document_init(&state->document))
    {
        return false;
    }

    cursor_init(&state->cursor);
    viewport_init(&state->viewport);
    state->is_dirty = false;
    state->file_path = NULL;
    state->message[0] = '\0';
    state->filename_entry_active = false;
    state->filename_entry[0] = '\0';
    state->filename_entry_length = 0;

    return true;
}

bool editor_state_load_file(EditorState *state, const char *path)
{
    if (state == NULL || path == NULL)
    {
        return false;
    }

    char *new_path;
    size_t path_length = strlen(path);

    if (path_length == SIZE_MAX)
    {
        return false;
    }

    new_path = malloc(path_length + 1);
    if (new_path == NULL)
    {
        return false;
    }
    memcpy(new_path, path, path_length + 1);

    /* Do not change the current state or path unless loading succeeds. */
    if (file_load_document(path, &state->document) != FILE_LOAD_OK)
    {
        free(new_path);
        return false;
    }

    free(state->file_path);
    state->file_path = new_path;
    cursor_init(&state->cursor);
    viewport_init(&state->viewport);
    state->is_dirty = false;
    state->filename_entry_active = false;
    state->filename_entry[0] = '\0';
    state->filename_entry_length = 0;

    return true;
}

bool editor_state_save_file(EditorState *state, const char *path)
{
    if (state == NULL || path == NULL)
    {
        return false;
    }

    if (file_save_document(path, &state->document) != FILE_SAVE_OK)
    {
        return false;
    }

    state->is_dirty = false;
    return true;
}

bool editor_state_save_as(EditorState *state, const char *path)
{
    char *new_path;
    size_t path_length;

    if (state == NULL || path == NULL || path[0] == '\0')
    {
        return false;
    }

    path_length = strlen(path);
    if (path_length == SIZE_MAX)
    {
        return false;
    }

    new_path = malloc(path_length + 1);
    if (new_path == NULL)
    {
        return false;
    }
    memcpy(new_path, path, path_length + 1);

    if (file_save_document(path, &state->document) != FILE_SAVE_OK)
    {
        free(new_path);
        return false;
    }

    free(state->file_path);
    state->file_path = new_path;
    state->is_dirty = false;
    return true;
}

bool editor_state_set_message(EditorState *state, const char *message)
{
    size_t length;

    if (state == NULL || message == NULL)
    {
        return false;
    }

    length = strlen(message);
    if (length >= sizeof(state->message))
    {
        length = sizeof(state->message) - 1;
    }

    memmove(state->message, message, length);
    state->message[length] = '\0';
    return true;
}

void editor_state_destroy(EditorState *state)
{
    if (state == NULL)
    {
        return;
    }

    document_destroy(&state->document);
    free(state->file_path);
    state->file_path = NULL;
    cursor_init(&state->cursor);
    viewport_init(&state->viewport);
    state->is_dirty = false;
    state->message[0] = '\0';
    state->filename_entry_active = false;
    state->filename_entry[0] = '\0';
    state->filename_entry_length = 0;
}