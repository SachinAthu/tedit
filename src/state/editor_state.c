#include "editor_state.h"
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
    state->configuration.tab_width = DEFAULT_TAB_WIDTH;
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
 * Adopt a successfully loaded Document and its allocated file association.
 * No file-system APIs are used in the Core Layer; loading and staging are
 * performed by the Application Layer before this state transition.
 */
bool editor_state_adopt_loaded_document(EditorState *state, Document *replacement, char *owned_file_path)
{
    if (state == NULL || replacement == NULL || owned_file_path == NULL)
    {
        return false;
    }

    document_destroy(&state->document);
    state->document = *replacement;
    memset(replacement, 0, sizeof(*replacement));

    free(state->file_path);
    state->file_path = owned_file_path;
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
 * Replace the file association using a path whose storage is already owned
 * by the caller. The transfer itself cannot fail, which lets Save As prepare
 * all required memory before committing the disk write.
 */
void editor_state_take_file_path(EditorState *state, char *owned_file_path)
{
    if (state == NULL)
    {
        free(owned_file_path);
        return;
    }

    if (state->file_path != owned_file_path)
    {
        free(state->file_path);
        state->file_path = owned_file_path;
    }
}

/*
 * Mark the current Document clean after a successful File Manager operation.
 */
void editor_state_mark_clean(EditorState *state)
{
    if (state != NULL)
    {
        state->is_dirty = false;
    }
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
