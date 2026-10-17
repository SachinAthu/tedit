#include "cursor.h"

/*
 * Place the cursor at the start of the document (first row, first column).
 *
 * Parameters:
 *   cursor - the cursor to initialize; may be NULL.
 *
 * Returns:
 *   void.
 */
void cursor_init(Cursor *cursor)
{
    if (cursor == NULL)
    {
        return;
    }

    cursor->row = 0;
    cursor->column = 0;
    cursor->preferred_column = 0;
}

/*
 * Move the cursor to an explicit position, but only if that position lies
 * inside the document and within the target line.
 *
 * Parameters:
 *   cursor   - the cursor to move.
 *   document - the document the cursor refers to.
 *   row      - the zero-based target line index.
 *   column   - the zero-based target column (0 to line length).
 *
 * Returns:
 *   true if the cursor was moved, false if `cursor` or `document` is NULL,
 *   `row` is out of range, or `column` is past the end of the line.
 */
bool cursor_set_position(Cursor *cursor, const Document *document, size_t row, size_t column)
{
    if (cursor == NULL || document == NULL || row >= document->line_count)
    {
        return false;
    }

    const DocumentLine *line = &document->lines[row];

    if (column > line->length)
    {
        return false;
    }

    cursor->row = row;
    cursor->column = column;
    cursor->preferred_column = column;

    return true;
}

/*
 * Move the cursor one column to the right, without leaving the current line.
 *
 * Parameters:
 *   cursor   - the cursor to move.
 *   document - the document the cursor refers to.
 *
 * Returns:
 *   true if the cursor advanced, false if `cursor` or `document` is NULL, the
 *   current row is out of range, or the cursor is already at the end of the
 *   line.
 */
bool cursor_advance(Cursor *cursor, const Document *document)
{
    const DocumentLine *line;

    if (cursor == NULL || document == NULL ||
        cursor->row >= document->line_count)
    {
        return false;
    }

    line = &document->lines[cursor->row];

    if (cursor->column >= line->length)
    {
        return false;
    }

    cursor->column += 1;
    cursor->preferred_column = cursor->column;

    return true;
}

/*
 * Move the cursor one column to the left, without leaving the current line.
 *
 * Parameters:
 *   cursor - the cursor to move.
 *
 * Returns:
 *   true if the cursor moved back, false if `cursor` is NULL or it is already
 *   at the start of the line.
 */
bool cursor_move_left(Cursor *cursor)
{
    if (cursor == NULL || cursor->column == 0)
    {
        return false;
    }

    cursor->column -= 1;
    cursor->preferred_column = cursor->column;
    return true;
}

/*
 * Move the cursor one column to the right, without passing the end of the current logical line.
 *
 * Parameters:
 *   cursor   - the cursor to move.
 *   document - the document the cursor refers to.
 *
 * Returns:
 *   true if the cursor advanced, false if `cursor` or `document` is NULL, the
 *   current row is out of range, or the cursor is already at the end of the
 *   line.
 */
bool cursor_move_right(Cursor *cursor, const Document *document)
{
    const DocumentLine *line;

    if (cursor == NULL || document == NULL || cursor->row >= document->line_count)
    {
        return false;
    }

    line = &document->lines[cursor->row];
    if (cursor->column >= line->length)
    {
        return false;
    }

    cursor->column += 1;
    cursor->preferred_column = cursor->column;
    return true;
}

/*
 * Move the cursor to the beginning of its current logical line.
 *
 * Parameters:
 *   cursor - the cursor to move.
 *
 * Returns:
 *   true if the cursor moved, false when cursor is NULL or already at column 0.
 */
bool cursor_move_home(Cursor *cursor)
{
    if (cursor == NULL || cursor->column == 0)
    {
        return false;
    }

    cursor->column = 0;
    cursor->preferred_column = 0;
    return true;
}

/*
 * Move the cursor to the end of its current logical line.
 *
 * Parameters:
 *   cursor   - the cursor to move.
 *   document - the document the cursor refers to.
 *
 * Returns:
 *   true if the cursor moved, false for invalid arguments or when already at
 *   the end of the current line.
 */
bool cursor_move_end(Cursor *cursor, const Document *document)
{
    const DocumentLine *line;

    if (cursor == NULL || document == NULL || cursor->row >= document->line_count)
    {
        return false;
    }

    line = &document->lines[cursor->row];
    if (cursor->column == line->length)
    {
        return false;
    }

    cursor->column = line->length;
    cursor->preferred_column = cursor->column;
    return true;
}

/*
 * Move the cursor to another line while preserving its preferred column.
 *
 * This is shared by cursor_move_up() and cursor_move_down(). It moves to
 * `target_row` and tries to keep the cursor at `preferred_column`; if the
 * target line is shorter than that, the cursor stops at the line's end.
 *
 * Parameters:
 *   cursor     - the cursor to move.
 *   document   - the document the cursor refers to.
 *   target_row - the zero-based line to move to.
 *
 * Returns:
 *   true if the cursor moved, false on NULL arguments or if `target_row` is
 *   out of range.
 */
static bool cursor_move_vertical(Cursor *cursor, const Document *document,
                                 size_t target_row)
{
    const DocumentLine *target_line;

    if (cursor == NULL || document == NULL ||
        target_row >= document->line_count)
    {
        return false;
    }

    target_line = &document->lines[target_row];
    cursor->row = target_row;
    cursor->column = cursor->preferred_column;

    if (cursor->column > target_line->length)
    {
        cursor->column = target_line->length;
    }

    return true;
}

/*
 * Move the cursor up one line, keeping the preferred column when possible.
 *
 * Parameters:
 *   cursor   - the cursor to move.
 *   document - the document the cursor refers to.
 *
 * Returns:
 *   true if the cursor moved up, false on NULL arguments or if the cursor is
 *   already on the first line.
 */
bool cursor_move_up(Cursor *cursor, const Document *document)
{
    if (cursor == NULL || document == NULL || cursor->row == 0 ||
        cursor->row >= document->line_count)
    {
        return false;
    }

    return cursor_move_vertical(cursor, document, cursor->row - 1);
}

/*
 * Move the cursor down one line, keeping the preferred column when possible.
 *
 * Parameters:
 *   cursor   - the cursor to move.
 *   document - the document the cursor refers to.
 *
 * Returns:
 *   true if the cursor moved down, false on NULL arguments or if the cursor is
 *   already on the last line.
 */
bool cursor_move_down(Cursor *cursor, const Document *document)
{
    if (cursor == NULL || document == NULL ||
        cursor->row >= document->line_count ||
        cursor->row + 1 >= document->line_count)
    {
        return false;
    }

    return cursor_move_vertical(cursor, document, cursor->row + 1);
}
