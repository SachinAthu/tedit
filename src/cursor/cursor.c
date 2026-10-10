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

    return true;
}
