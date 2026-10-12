#ifndef STATE_EDITOR_STATE_H
#define STATE_EDITOR_STATE_H

#include "cursor/cursor.h"
#include "document/document.h"
#include "viewport/viewport.h"

#include <stdbool.h>

typedef struct
{
    Document document;
    Cursor cursor;
    Viewport viewport;
    bool is_dirty;
} EditorState;

bool editor_state_init(EditorState *state);

void editor_state_destroy(EditorState *state);

#endif