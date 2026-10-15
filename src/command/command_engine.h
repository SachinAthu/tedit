#ifndef COMMAND_COMMAND_ENGINE_H
#define COMMAND_COMMAND_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#include "state/editor_state.h"

/*
 * The actions the Command Engine can perform on the editor state.
 *
 * EDITOR_COMMAND_NONE is a no-op used when an input event maps to no action.
 */
typedef enum
{
    EDITOR_COMMAND_NONE = 0,
    EDITOR_COMMAND_INSERT_CHARACTER,
    EDITOR_COMMAND_BACKSPACE,
    EDITOR_COMMAND_INSERT_NEWLINE,
    EDITOR_COMMAND_CANCEL,
    EDITOR_COMMAND_MOVE_LEFT,
    EDITOR_COMMAND_MOVE_RIGHT,
    EDITOR_COMMAND_MOVE_UP,
    EDITOR_COMMAND_MOVE_DOWN,
    EDITOR_COMMAND_DELETE_CHARACTER,
    EDITOR_COMMAND_SAVE,
    EDITOR_COMMAND_QUIT
} EditorCommandType;

/*
 * One command to execute.
 *
 * `character` holds the Unicode scalar only for
 * EDITOR_COMMAND_INSERT_CHARACTER; for every other command it is unused (0).
 */
typedef struct
{
    EditorCommandType type;
    uint32_t character;
} EditorCommand;

/*
 * Execute one command against the editor state.
 *
 * The command may edit the document, move the cursor, save the file, open
 * the filename-entry prompt, or start/answer the quit confirmation. The
 * caller uses the out-parameters to decide whether to redraw and whether to
 * leave the event loop.
 *
 * Parameters:
 *   state       - the editor state to modify; must not be NULL.
 *   command     - the command to execute; must not be NULL.
 *   save_path   - path passed on the command line, used as the save target
 *                 when the document has no file path of its own yet; may be
 *                 NULL.
 *   redraw      - out-parameter: set to true when the screen should be
 *                 redrawn; must not be NULL.
 *   should_quit - out-parameter: set to true when the editor should exit;
 *                 must not be NULL.
 *
 * Returns:
 *   true on success, false on NULL arguments. On success `redraw` and
 *   `should_quit` are always written (false when the command did nothing).
 */
bool command_engine_execute(EditorState *state,
                            const EditorCommand *command,
                            const char *save_path,
                            bool *redraw,
                            bool *should_quit);

#endif
