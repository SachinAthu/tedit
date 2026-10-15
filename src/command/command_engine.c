#include "command/command_engine.h"

#include "cursor/cursor.h"
#include "document/document.h"
#include "state/editor_state.h"

static bool command_backspace(EditorState *state, bool *changed)
{
    if (state->cursor.column > 0)
    {
        const size_t delete_column = state->cursor.column - 1;
        if (!document_delete_character(&state->document, state->cursor.row, delete_column))
        {
            return true;
        }
        if (!cursor_set_position(&state->cursor, &state->document,
                                 state->cursor.row, delete_column))
        {
            return false;
        }
    }
    else if (state->cursor.row > 0)
    {
        const size_t previous_row = state->cursor.row - 1;
        const DocumentLine *previous_line = document_get_line(&state->document, previous_row);
        if (previous_line == NULL)
        {
            return true;
        }

        const size_t previous_length = previous_line->length;
        if (!document_merge_lines(&state->document, previous_row))
        {
            return true;
        }
        if (!cursor_set_position(&state->cursor, &state->document,
                                 previous_row, previous_length))
        {
            return false;
        }
    }
    else
    {
        return true;
    }

    state->is_dirty = true;
    *changed = true;
    return true;
}

static size_t encode_utf8(uint32_t character, unsigned char bytes[4])
{
    if (character <= 0x7F)
    {
        bytes[0] = (unsigned char)character;
        return 1;
    }
    if (character <= 0x7FF)
    {
        bytes[0] = (unsigned char)(0xC0 | (character >> 6));
        bytes[1] = (unsigned char)(0x80 | (character & 0x3F));
        return 2;
    }
    if (character <= 0xFFFF && !(character >= 0xD800 && character <= 0xDFFF))
    {
        bytes[0] = (unsigned char)(0xE0 | (character >> 12));
        bytes[1] = (unsigned char)(0x80 | ((character >> 6) & 0x3F));
        bytes[2] = (unsigned char)(0x80 | (character & 0x3F));
        return 3;
    }
    if (character <= 0x10FFFF)
    {
        bytes[0] = (unsigned char)(0xF0 | (character >> 18));
        bytes[1] = (unsigned char)(0x80 | ((character >> 12) & 0x3F));
        bytes[2] = (unsigned char)(0x80 | ((character >> 6) & 0x3F));
        bytes[3] = (unsigned char)(0x80 | (character & 0x3F));
        return 4;
    }
    return 0;
}

static bool filename_entry_execute(EditorState *state,
                                   const EditorCommand *command,
                                   bool *redraw)
{
    if (command->type == EDITOR_COMMAND_CANCEL)
    {
        state->filename_entry_active = false;
        state->filename_entry_length = 0;
        state->filename_entry[0] = '\0';
        (void)editor_state_set_message(state, "Save As cancelled.");
        *redraw = true;
        return true;
    }

    if (command->type == EDITOR_COMMAND_BACKSPACE)
    {
        if (state->filename_entry_length > 0)
        {
            --state->filename_entry_length;
            while (state->filename_entry_length > 0 &&
                   (((unsigned char)state->filename_entry[state->filename_entry_length] & 0xC0) == 0x80))
            {
                --state->filename_entry_length;
            }
            state->filename_entry[state->filename_entry_length] = '\0';
            *redraw = true;
        }
        return true;
    }

    if (command->type == EDITOR_COMMAND_INSERT_CHARACTER)
    {
        unsigned char encoded[4];
        const size_t encoded_length = encode_utf8(command->character, encoded);

        if (encoded_length == 0)
        {
            return true;
        }
        if (encoded_length > EDITOR_FILENAME_CAPACITY - 1 - state->filename_entry_length)
        {
            (void)editor_state_set_message(state, "Filename is too long.");
            *redraw = true;
            return true;
        }

        for (size_t index = 0; index < encoded_length; ++index)
        {
            state->filename_entry[state->filename_entry_length++] = (char)encoded[index];
        }
        state->filename_entry[state->filename_entry_length] = '\0';
        *redraw = true;
        return true;
    }

    if (command->type == EDITOR_COMMAND_INSERT_NEWLINE)
    {
        if (state->filename_entry_length == 0)
        {
            (void)editor_state_set_message(state, "Enter a file name.");
            *redraw = true;
            return true;
        }

        if (editor_state_save_as(state, state->filename_entry))
        {
            state->filename_entry_active = false;
            state->filename_entry_length = 0;
            state->filename_entry[0] = '\0';
            (void)editor_state_set_message(state, "Saved successfully.");
        }
        else
        {
            (void)editor_state_set_message(
                state, "Save failed; check path and try again, or Esc to cancel.");
        }
        *redraw = true;
        return true;
    }

    /* Ignore navigation and other commands while filename entry is active. */
    return true;
}

bool command_engine_execute(EditorState *state,
                            const EditorCommand *command,
                            const char *save_path,
                            bool *redraw,
                            bool *should_quit)
{
    bool changed = false;

    if (state == NULL || command == NULL || redraw == NULL || should_quit == NULL)
    {
        return false;
    }

    *redraw = false;
    *should_quit = false;

    if (state->filename_entry_active)
    {
        return filename_entry_execute(state, command, redraw);
    }

    switch (command->type)
    {
    case EDITOR_COMMAND_NONE:
        break;
    case EDITOR_COMMAND_QUIT:
        *should_quit = true;
        break;
    case EDITOR_COMMAND_SAVE:
    {
        const char *current_path = state->file_path != NULL ? state->file_path : save_path;

        if (current_path == NULL)
        {
            state->filename_entry_active = true;
            state->filename_entry_length = 0;
            state->filename_entry[0] = '\0';
            (void)editor_state_set_message(state, "Enter: save  Esc: cancel");
        }
        else if (editor_state_save_file(state, current_path))
        {
            (void)editor_state_set_message(state, "Saved successfully.");
        }
        else
        {
            (void)editor_state_set_message(state, "Save failed; document remains modified.");
        }
        *redraw = true;
        break;
    }
    case EDITOR_COMMAND_CANCEL:
        break;
    case EDITOR_COMMAND_INSERT_CHARACTER:
        if (!document_insert_character(&state->document, state->cursor.row,
                                       state->cursor.column, command->character))
        {
            return true;
        }
        if (!cursor_advance(&state->cursor, &state->document))
        {
            return false;
        }
        state->is_dirty = true;
        changed = true;
        break;
    case EDITOR_COMMAND_BACKSPACE:
        if (!command_backspace(state, &changed))
        {
            return false;
        }
        break;
    case EDITOR_COMMAND_INSERT_NEWLINE:
        if (!document_split_line(&state->document, state->cursor.row, state->cursor.column))
        {
            return true;
        }
        if (!cursor_set_position(&state->cursor, &state->document,
                                 state->cursor.row + 1, 0))
        {
            return false;
        }
        state->is_dirty = true;
        changed = true;
        break;
    case EDITOR_COMMAND_MOVE_LEFT:
        changed = cursor_move_left(&state->cursor);
        break;
    case EDITOR_COMMAND_MOVE_RIGHT:
        changed = cursor_move_right(&state->cursor, &state->document);
        break;
    case EDITOR_COMMAND_MOVE_UP:
        changed = cursor_move_up(&state->cursor, &state->document);
        break;
    case EDITOR_COMMAND_MOVE_DOWN:
        changed = cursor_move_down(&state->cursor, &state->document);
        break;
    default:
        return false;
    }

    *redraw = *redraw || changed;
    return true;
}