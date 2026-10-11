#include "editor_state.h"

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
void editor_state_destroy(EditorState *state)
{
    if (state == NULL)
    {
        return;
    }

    document_destroy(&state->document);
    cursor_init(&state->cursor);
    state->is_dirty = false;
}
