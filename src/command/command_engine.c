#include "command/command_engine.h"

#include "cursor/cursor.h"
#include "document/document.h"
#include "state/editor_state.h"

#include <string.h>

/*
 * This file is the "brain" of the editor: it turns commands into concrete
 * changes to the editor state. All document edits, cursor moves, saves, and
 * the quit flow run through command_engine_execute().
 *
 * Two small "modes" are handled here as well: filename entry (open after
 * Ctrl-S on an unnamed document) and the quit confirmation (open after
 * Ctrl-Q on a dirty document). While a mode is active, commands are routed
 * to that mode's handler instead of the normal command switch.
 */

/*
 * Perform a Backspace edit in the document.
 *
 * Deletes the code point before the cursor, or - when the cursor is at the
 * start of a line that has one above it - merges the two lines and leaves
 * the cursor at the join point.
 *
 * Parameters:
 *   state   - the editor state to modify.
 *   changed - out-parameter: set to true when the document changed.
 *
 * Returns:
 *   true when the command was handled (including "nothing to delete" and
 *   "deletion failed", which simply leave the document alone), false on an
 *   internal cursor-update failure.
 */
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
        if (!cursor_set_position(&state->cursor, &state->document, previous_row, previous_length))
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

/*
 * Encode one Unicode scalar as UTF-8.
 *
 * Parameters:
 *   character - the scalar to encode.
 *   bytes     - out-parameter: receives the encoded bytes (1..4).
 *
 * Returns:
 *   the number of bytes written, or 0 if `character` is not a valid scalar.
 */
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

/*
 * Return the byte index at the start of the UTF-8 code point just before
 * `position` in `text`.
 *
 * Used to move the filename-entry cursor left by one character rather than
 * by one byte.
 *
 * Parameters:
 *   text     - the UTF-8 string to walk.
 *   position - the current cursor byte index.
 *
 * Returns:
 *   the byte index of the previous code point, or 0 when already at the
 *   start.
 */
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

/*
 * Return the byte index at the start of the UTF-8 code point just after
 * `position` in `text`.
 *
 * Used to move the filename-entry cursor right by one character rather than
 * by one byte.
 *
 * Parameters:
 *   text     - the UTF-8 string to walk.
 *   length   - the number of bytes in use in `text`.
 *   position - the current cursor byte index.
 *
 * Returns:
 *   the byte index of the next code point, or `length` when already at the
 *   end.
 */
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

/*
 * Close the filename-entry prompt, discarding what was typed.
 *
 * Parameters:
 *   state - the editor state to update.
 *
 * Returns:
 *   void.
 */
static void filename_entry_clear(EditorState *state)
{
    state->filename_entry_active = false;
    state->filename_entry_length = 0;
    state->filename_entry_cursor = 0;
    state->filename_entry[0] = '\0';
}

/*
 * Open the filename-entry prompt with an empty entry.
 *
 * Parameters:
 *   state          - the editor state to update.
 *   quit_after_save - whether a successful save should also quit the editor
 *                     (true when the prompt was opened from the quit flow).
 *
 * Returns:
 *   void.
 */
static void filename_entry_start(EditorState *state, bool quit_after_save)
{
    state->filename_entry_active = true;
    state->filename_entry_length = 0;
    state->filename_entry_cursor = 0;
    state->filename_entry[0] = '\0';
    state->quit_after_save = quit_after_save;
    (void)editor_state_set_message(state, "Enter: save  Esc: cancel");
}

/*
 * Perform a Ctrl-S save.
 *
 * A document with a known path is written back to it. An unnamed document
 * has nowhere to go yet, so the filename-entry prompt is opened instead.
 * When `quitting` is true and the save succeeds, the editor quits.
 *
 * Parameters:
 *   state       - the editor state to read and update.
 *   save_path   - fallback path from the command line; may be NULL.
 *   quitting    - whether this save was triggered by the quit flow.
 *   redraw      - out-parameter: set to true (the status message changed).
 *   should_quit - out-parameter: set to true when the editor should exit.
 *
 * Returns:
 *   always true.
 */
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

/*
 * Route a command through the active filename-entry prompt.
 *
 * While the prompt is open, every keystroke edits the filename: printable
 * characters are inserted at the cursor, arrows and Backspace/Delete edit
 * the entry, Enter saves under the typed name (quitting as well when the
 * prompt was opened from the quit flow), and Escape cancels. Anything else
 * is ignored.
 *
 * Parameters:
 *   state       - the editor state to update.
 *   command     - the command to handle.
 *   redraw      - out-parameter: set to true when the entry or message
 *                 changed.
 *   should_quit - out-parameter: set to true when a save-and-quit finishes.
 *
 * Returns:
 *   always true.
 */
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
            *redraw = true;
        }
        return true;
    }

    if (command->type == EDITOR_COMMAND_BACKSPACE)
    {
        if (state->filename_entry_cursor > 0)
        {
            const size_t previous = filename_previous_boundary(state->filename_entry, state->filename_entry_cursor);
            const size_t removed = state->filename_entry_cursor - previous;
            memmove(state->filename_entry + previous,
                    state->filename_entry + state->filename_entry_cursor,
                    state->filename_entry_length - state->filename_entry_cursor + 1);
            state->filename_entry_length -= removed;
            state->filename_entry_cursor = previous;
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

/*
 * Route a command through the active quit-confirmation prompt.
 *
 * The prompt answers Ctrl-Q on a dirty document: Y saves first (opening
 * filename entry if the document is unnamed) and then quits, N discards the
 * changes and quits, and Escape cancels. Other keys are ignored.
 *
 * Parameters:
 *   state       - the editor state to update.
 *   command     - the command to handle.
 *   save_path   - fallback path from the command line; may be NULL.
 *   redraw      - out-parameter: set to true when the prompt state changed.
 *   should_quit - out-parameter: set to true when the editor should exit.
 *
 * Returns:
 *   always true.
 */
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
            (void)editor_state_set_message(state, "Save changes before quitting?");
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
        /* Delete currently edits only the filename-entry prompt; deleting
         * inside the document itself is deferred, so nothing happens here. */
        break;
    default:
        return false;
    }

    *redraw = *redraw || changed;
    return true;
}
