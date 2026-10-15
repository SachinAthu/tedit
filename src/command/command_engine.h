#ifndef COMMAND_COMMAND_ENGINE_H
#define COMMAND_COMMAND_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#include "state/editor_state.h"

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

typedef struct
{
    EditorCommandType type;
    uint32_t character;
} EditorCommand;

/* Execute one command. redraw and should_quit are always initialized on success. */
bool command_engine_execute(EditorState *state,
                            const EditorCommand *command,
                            const char *save_path,
                            bool *redraw,
                            bool *should_quit);

#endif
