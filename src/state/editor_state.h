#ifndef STATE_EDITOR_STATE_H
#define STATE_EDITOR_STATE_H

#include "cursor/cursor.h"
#include "document/document.h"
#include "viewport/viewport.h"

#include <stdbool.h>

/*
 * The single, top-level piece of state that describes the whole editor.
 *
 * It bundles together the three things the editor tracks as the user works:
 *   - document: the text being edited,
 *   - cursor:   where the text cursor currently is,
 *   - viewport: which part of the document is visible on screen.
 *
 * `is_dirty` is a flag that becomes true whenever the document has been
 * changed but not yet saved to disk.
 */
typedef struct
{
    Document document;
    Cursor cursor;
    Viewport viewport;
    bool is_dirty;
} EditorState;

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
bool editor_state_init(EditorState *state);

/*
 * Release everything the editor state owns and reset it to an empty state.
 *
 * Parameters:
 *   state - the editor state to destroy; may be NULL.
 */
void editor_state_destroy(EditorState *state);

#endif