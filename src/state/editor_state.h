#ifndef STATE_EDITOR_STATE_H
#define STATE_EDITOR_STATE_H

#include "cursor/cursor.h"
#include "document/document.h"
#include "viewport/viewport.h"

#include <stdbool.h>

/* The top-level state of the editor: the document being edited, the cursor,
 * the scroll position, and a flag marking whether there are unsaved changes. */
typedef struct
{
    Document document;   /* The text being edited. */
    Cursor cursor;       /* The current cursor position within the document. */
    Viewport viewport;   /* The screen's scroll offset. */
    bool is_dirty;       /* True when the document has unsaved changes. */
} EditorState;

bool editor_state_init(EditorState *state);

void editor_state_destroy(EditorState *state);

#endif