#ifndef STATE_EDITOR_STATE_H
#define STATE_EDITOR_STATE_H

#include "cursor/cursor.h"
#include "document/document.h"
#include "viewport/viewport.h"

#include <stdbool.h>
#include <stddef.h>

#define EDITOR_MESSAGE_CAPACITY 256
#define EDITOR_FILENAME_CAPACITY 4096

typedef struct
{
    Document document;
    Cursor cursor;
    Viewport viewport;
    bool is_dirty;
    char *file_path;
    char message[EDITOR_MESSAGE_CAPACITY];
    bool filename_entry_active;
    char filename_entry[EDITOR_FILENAME_CAPACITY];
    size_t filename_entry_length;
    size_t filename_entry_cursor;
    bool quit_confirmation_active;
    bool quit_after_save;
} EditorState;

bool editor_state_init(EditorState *state);

bool editor_state_load_file(EditorState *state, const char *path);

bool editor_state_save_file(EditorState *state, const char *path);

bool editor_state_save_as(EditorState *state, const char *path);

bool editor_state_set_message(EditorState *state, const char *message);

void editor_state_destroy(EditorState *state);

#endif