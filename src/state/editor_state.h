#ifndef STATE_EDITOR_STATE_H
#define STATE_EDITOR_STATE_H

#include "cursor/cursor.h"
#include "document/document.h"
#include "viewport/viewport.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    Document document;
    Cursor cursor;
    Viewport viewport;
    bool is_dirty;
    char *file_path;
} EditorState;

bool editor_state_init(EditorState *state);

bool editor_state_load_file(EditorState *state, const char *path);

bool editor_state_save_file(EditorState *state, const char *path);

void editor_state_destroy(EditorState *state);

#endif