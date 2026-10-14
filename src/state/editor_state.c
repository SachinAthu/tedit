#include "editor_state.h"
#include "file/file.h"

#include <string.h>

/*
 * Create the initial editor state: an empty document with one blank line and
 * a cursor placed at the start.
 *
 * Parameters:
 *   state - the editor state to initialize.
 *
 * Returns:
 *   true on success, false if `state` is NULL or the document fails to
 *   initialize.
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

    return true;
}

/*
 * Load a file and reset the session's initial editing position.
 *
 * Parameters:
 *   state - initialized Editor State to update.
 *   path  - path of the file to load.
 *
 * Returns:
 *   true on success; false on invalid arguments or any file-loading failure.
 *   The existing Editor State is preserved when loading fails.
 */
bool editor_state_load_file(EditorState *state, const char *path)
{
    if (state == NULL || path == NULL)
    {
        return false;
    }

    /*
     * The File Manager stages the complete load and changes the Document
     * only on success. Do not reset the rest of the Editor State until then.
     */
    if (file_load_document(path, &state->document) != FILE_LOAD_OK)
    {
        return false;
    }

    cursor_init(&state->cursor);
    viewport_init(&state->viewport);
    state->is_dirty = false;

    return true;
}

/*
 * Release everything the editor state owns and reset it to an empty state.
 *
 * Parameters:
 *   state - the editor state to destroy; may be NULL.
 *
 * Returns:
 *   void.
 */
/*
 * Save the current Document through the File Manager.
 * The Document and Dirty Flag remain unchanged on every failure.
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

void editor_state_destroy(EditorState *state)
{
    if (state == NULL)
    {
        return;
    }

    document_destroy(&state->document);
    cursor_init(&state->cursor);
    viewport_init(&state->viewport);
    state->is_dirty = false;
}
