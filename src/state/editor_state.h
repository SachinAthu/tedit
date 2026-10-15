#ifndef STATE_EDITOR_STATE_H
#define STATE_EDITOR_STATE_H

#include "cursor/cursor.h"
#include "document/document.h"
#include "viewport/viewport.h"

#include <stdbool.h>
#include <stddef.h>

#define EDITOR_MESSAGE_CAPACITY 256
#define EDITOR_FILENAME_CAPACITY 4096

/*
 * The single source of truth for everything the editor knows.
 *
 * All modules read and change application data through this structure. Each
 * field has exactly one owning module (see context/02-system-architecture.md);
 * every other module may read it but must not modify it directly.
 *
 *   - document:               the text being edited, held in memory.
 *   - cursor:                 where the next edit happens (row, column).
 *   - viewport:               which part of the document is on screen.
 *   - is_dirty:               true while the document has unsaved changes.
 *   - file_path:              path of the loaded file, or NULL for a new
 *                             untitled document.
 *   - message:                one-line status text shown in the Message Bar.
 *   - filename_entry_active:  true while the "Save as:" prompt is capturing
 *                             keystrokes.
 *   - filename_entry:         text typed into the filename prompt, stored as
 *                             UTF-8 bytes.
 *   - filename_entry_length:  number of bytes used in filename_entry.
 *   - filename_entry_cursor:  byte offset of the cursor within filename_entry
 *                             (always on a UTF-8 code-point boundary).
 *   - quit_confirmation_active: true while the "save before quitting?" prompt
 *                             is shown for a dirty document.
 *   - quit_after_save:        true when filename entry was opened by Quit, so
 *                             the editor quits after a successful save.
 */
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
bool editor_state_init(EditorState *state);

/*
 * Replace the current document with the contents of the UTF-8 file at `path`.
 *
 * The current document, file association, and dirty flag are kept until the
 * file has been read and decoded successfully; on failure nothing changes.
 *
 * Parameters:
 *   state - the state to update; must not be NULL.
 *   path  - path of the file to load; must not be NULL.
 *
 * Returns:
 *   true on success, false on NULL arguments, allocation failure, or if the
 *   file could not be opened, read, or decoded.
 */
bool editor_state_load_file(EditorState *state, const char *path);

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
bool editor_state_save_file(EditorState *state, const char *path);

/*
 * Write the document to a new path and remember that path as the file
 * association.
 *
 * Parameters:
 *   state - the state to update; must not be NULL.
 *   path  - destination path; must not be NULL or empty.
 *
 * Returns:
 *   true if the file was written, the new path became the association, and
 *   the dirty flag was cleared; false otherwise. On failure the old
 *   association and the dirty flag are preserved.
 */
bool editor_state_save_as(EditorState *state, const char *path);

/*
 * Copy `message` into the state's Message Bar text, truncating it if it is
 * longer than the message buffer.
 *
 * Parameters:
 *   state   - the state to update; must not be NULL.
 *   message - the text to show; must not be NULL.
 *
 * Returns:
 *   true on success, false on NULL arguments.
 */
bool editor_state_set_message(EditorState *state, const char *message);

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
void editor_state_destroy(EditorState *state);

#endif