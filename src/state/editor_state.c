#include "editor_state.h"
#include "file/file.h"

#include <string.h>
#include <stdlib.h>

/*
 * Initialize an EditorState to a fresh, empty, not-dirty document.
 *
 * Parameters:
 *   state - the state to initialize; must not be NULL.
 *
 * Returns:
 *   true on success, false if `state` is NULL or allocation failed. On
 *   failure the state is left zeroed and safe to inspect.
 */
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
    state->filename_entry_cursor = 0;
    state->quit_confirmation_active = false;
    state->quit_after_save = false;

    return true;
}

/*
 * Replace the current document with the contents of the UTF-8 file at `path`.
 *
 * The path string is copied up front (so the caller may free its own), but
 * the current document, file association, and dirty flag are only replaced
 * after the file loads and decodes successfully; on failure nothing changes.
 *
 * Parameters:
 *   state - the state to update; must not be NULL.
 *   path  - path of the file to load; must not be NULL.
 *
 * Returns:
 *   true on success, false on NULL arguments, allocation failure, or if the
 *   file could not be opened, read, or decoded.
 */
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
    state->filename_entry_cursor = 0;
    state->quit_confirmation_active = false;
    state->quit_after_save = false;

    return true;
}

/*
 * Write the document to the file at `path` and clear the dirty flag.
 *
 * Parameters:
 *   state - the state whose document is saved; must not be NULL.
 *   path  - destination path; must not be NULL.
 *
 * Returns:
 *   true if the file was written and the dirty flag cleared, false on NULL
 *   arguments or a write failure. On failure the dirty flag stays set so
 *   unsaved work is never silently lost.
 */
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

/*
 * Write the document to a new path and remember that path as the file
 * association.
 *
 * The new path is allocated before anything is written, and the old
 * association is replaced only after the save succeeds - so a failed save
 * keeps both the document and its previous file association intact.
 *
 * Parameters:
 *   state - the state to update; must not be NULL.
 *   path  - destination path; must not be NULL or empty.
 *
 * Returns:
 *   true if the file was written, the new path became the association, and
 *   the dirty flag was cleared; false otherwise.
 */
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

/*
 * Copy `message` into the state's Message Bar text.
 *
 * Parameters:
 *   state   - the state to update; must not be NULL.
 *   message - the text to show; must not be NULL.
 *
 * Returns:
 *   true on success, false on NULL arguments.
 */
bool editor_state_set_message(EditorState *state, const char *message)
{
    size_t length;

    if (state == NULL || message == NULL)
    {
        return false;
    }

    /* Copy at most a full buffer's worth, leaving room for the terminator. */
    length = strlen(message);
    if (length >= sizeof(state->message))
    {
        length = sizeof(state->message) - 1;
    }

    memmove(state->message, message, length);
    state->message[length] = '\0';
    return true;
}

/*
 * Release all storage owned by the state and reset it to a safe, empty
 * state.
 *
 * Parameters:
 *   state - the state to destroy; may be NULL.
 *
 * Returns:
 *   void.
 */
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
    state->filename_entry_cursor = 0;
    state->quit_confirmation_active = false;
    state->quit_after_save = false;
}
