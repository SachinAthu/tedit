#include "command/command_engine.h"

#include "cursor/cursor.h"
#include "document/document.h"
#include "state/editor_state.h"

#include <string.h>

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

static size_t filename_previous_boundary(const char *text, size_t position)
{
    if (position == 0)
    {
        return 0;
    }

    --position;
    while (position > 0 && (((unsigned char)text[position] & 0xC0) == 0x80))
    {
        --position;
    }
    return position;
}

static size_t filename_next_boundary(const char *text, size_t length, size_t position)
{
    if (position >= length)
    {
        return length;
    }

    ++position;
    while (position < length && (((unsigned char)text[position] & 0xC0) == 0x80))
    {
        ++position;
    }
    return position;
}

static void filename_entry_clear(EditorState *state)
{
    state->filename_entry_active = false;
    state->filename_entry_length = 0;
    state->filename_entry_cursor = 0;
    state->filename_entry[0] = '\0';
}

static void filename_entry_start(EditorState *state, bool quit_after_save)
{
    state->filename_entry_active = true;
    state->filename_entry_length = 0;
    state->filename_entry_cursor = 0;
    state->filename_entry[0] = '\0';
    state->quit_after_save = quit_after_save;
    (void)editor_state_set_message(state, "Enter: save  Esc: cancel");
}

static bool command_save(EditorState *state, const char *save_path, bool quitting, bool *redraw, bool *should_quit)
{
    const char *current_path = state->file_path != NULL ? state->file_path : save_path;

    if (current_path == NULL)
    {
        filename_entry_start(state, quitting);
        *redraw = true;
        return true;
    }

    if (editor_state_save_file(state, current_path))
    {
        (void)editor_state_set_message(state, "Saved successfully.");
        state->quit_after_save = false;
        if (quitting)
        {
            *should_quit = true;
        }
    }
    else
    {
        (void)editor_state_set_message(state, "Save failed; document remains modified.");
        state->quit_after_save = false;
    }

    *redraw = true;
    return true;
}

static bool filename_entry_execute(EditorState *state, const EditorCommand *command, bool *redraw, bool *should_quit)
{
    if (command->type == EDITOR_COMMAND_CANCEL)
    {
        const bool was_quitting = state->quit_after_save;
        filename_entry_clear(state);
        state->quit_after_save = false;
        (void)editor_state_set_message(state, was_quitting ? "Quit cancelled." : "Save As cancelled.");
        *redraw = true;
        return true;
    }

    if (command->type == EDITOR_COMMAND_MOVE_LEFT)
    {
        const size_t previous = filename_previous_boundary(state->filename_entry,
                                                           state->filename_entry_cursor);
        if (previous != state->filename_entry_cursor)
        {
            state->filename_entry_cursor = previous;
            (void)editor_state_set_message(state, "Enter: save  Esc: cancel");
            *redraw = true;
        }
        return true;
    }

    if (command->type == EDITOR_COMMAND_MOVE_RIGHT)
    {
        const size_t next = filename_next_boundary(state->filename_entry,
                                                   state->filename_entry_length,
                                                   state->filename_entry_cursor);
        if (next != state->filename_entry_cursor)
        {
            state->filename_entry_cursor = next;
            (void)editor_state_set_message(state, "Enter: save  Esc: cancel");
            *redraw = true;
        }
        return true;
    }

    if (command->type == EDITOR_COMMAND_BACKSPACE)
    {
        if (state->filename_entry_cursor > 0)
        {
            const size_t previous = filename_previous_boundary(state->filename_entry,
                                                               state->filename_entry_cursor);
            const size_t removed = state->filename_entry_cursor - previous;
            memmove(state->filename_entry + previous,
                    state->filename_entry + state->filename_entry_cursor,
                    state->filename_entry_length - state->filename_entry_cursor + 1);
            state->filename_entry_length -= removed;
            state->filename_entry_cursor = previous;
            (void)editor_state_set_message(state, "Enter: save  Esc: cancel");
            *redraw = true;
        }
        return true;
    }

    if (command->type == EDITOR_COMMAND_DELETE_CHARACTER)
    {
        if (state->filename_entry_cursor < state->filename_entry_length)
        {
            const size_t next = filename_next_boundary(state->filename_entry,
                                                       state->filename_entry_length,
                                                       state->filename_entry_cursor);
            const size_t removed = next - state->filename_entry_cursor;
            memmove(state->filename_entry + state->filename_entry_cursor,
                    state->filename_entry + next,
                    state->filename_entry_length - next + 1);
            state->filename_entry_length -= removed;
            (void)editor_state_set_message(state, "Enter: save  Esc: cancel");
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

        memmove(state->filename_entry + state->filename_entry_cursor + encoded_length,
                state->filename_entry + state->filename_entry_cursor,
                state->filename_entry_length - state->filename_entry_cursor + 1);
        memcpy(state->filename_entry + state->filename_entry_cursor, encoded, encoded_length);
        state->filename_entry_length += encoded_length;
        state->filename_entry_cursor += encoded_length;
        (void)editor_state_set_message(state, "Enter: save  Esc: cancel");
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
            const bool should_quit_after_save = state->quit_after_save;
            filename_entry_clear(state);
            state->quit_after_save = false;
            (void)editor_state_set_message(state, "Saved successfully.");
            if (should_quit_after_save)
            {
                *should_quit = true;
            }
        }
        else
        {
            (void)editor_state_set_message(
                state, "Save failed; check path and try again, or Esc to cancel.");
        }
        *redraw = true;
        return true;
    }

    /* Ignore commands not meaningful while a filename is being entered. */
    return true;
}

static bool quit_confirmation_execute(EditorState *state, const EditorCommand *command, const char *save_path, bool *redraw, bool *should_quit)
{
    if (command->type == EDITOR_COMMAND_CANCEL)
    {
        state->quit_confirmation_active = false;
        (void)editor_state_set_message(state, "Quit cancelled.");
        *redraw = true;
        return true;
    }

    if (command->type != EDITOR_COMMAND_INSERT_CHARACTER)
    {
        return true;
    }

    if (command->character == 'n' || command->character == 'N')
    {
        state->quit_confirmation_active = false;
        state->quit_after_save = false;
        *should_quit = true;
        return true;
    }

    if (command->character == 'y' || command->character == 'Y')
    {
        state->quit_confirmation_active = false;
        return command_save(state, save_path, true, redraw, should_quit);
    }

    return true;
}

bool command_engine_execute(EditorState *state, const EditorCommand *command, const char *save_path, bool *redraw, bool *should_quit)
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
        return filename_entry_execute(state, command, redraw, should_quit);
    }

    if (state->quit_confirmation_active)
    {
        return quit_confirmation_execute(state, command, save_path, redraw, should_quit);
    }

    switch (command->type)
    {
    case EDITOR_COMMAND_NONE:
        break;
    case EDITOR_COMMAND_QUIT:
        if (state->is_dirty)
        {
            state->quit_confirmation_active = true;
            state->quit_after_save = false;
            (void)editor_state_set_message(state, "Save changes before quitting? Y=save N=discard Esc=cancel");
            *redraw = true;
        }
        else
        {
            *should_quit = true;
        }
        break;
    case EDITOR_COMMAND_SAVE:
        return command_save(state, save_path, false, redraw, should_quit);
    case EDITOR_COMMAND_CANCEL:
        break;
    case EDITOR_COMMAND_INSERT_CHARACTER:
        if (!document_insert_character(&state->document, state->cursor.row, state->cursor.column, command->character))
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
        if (!cursor_set_position(&state->cursor, &state->document, state->cursor.row + 1, 0))
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
    case EDITOR_COMMAND_DELETE_CHARACTER:
        /* Document Delete is a navigation/editing milestone; ignore it here. */
        break;
    default:
        return false;
    }

    *redraw = *redraw || changed;
    return true;
}
