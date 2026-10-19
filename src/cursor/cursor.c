#include "cursor.h"
#include "text/text_layout.h"

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
    cursor->preferred_visual_column = 0;
    cursor->preferred_visual_column_valid = false;
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
    cursor->preferred_visual_column = column;
    cursor->preferred_visual_column_valid = false;

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
    cursor->preferred_visual_column_valid = false;

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
    cursor->preferred_visual_column_valid = false;
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
    cursor->preferred_visual_column_valid = false;
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
    cursor->preferred_visual_column_valid = false;
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
    cursor->preferred_visual_column_valid = false;
    return true;
}

/*
 * Recompute the remembered visual column from the current logical position.
 *
 * The preferred visual column is the display column the cursor keeps when
 * moving vertically across lines of differing tab/width layout. This
 * function (re)derives it from the cursor's current row and column, then
 * marks the stored value valid.
 *
 * Parameters:
 *   cursor    - the cursor to update.
 *   document  - the document the cursor refers to.
 *   tab_width - the configured tab stop width; must be greater than 0.
 *
 * Returns:
 *   true on success, false on NULL arguments, a tab width of 0, a cursor row
 *   out of range, or an inconsistent line.
 */
bool cursor_update_preferred_visual_column(Cursor *cursor, const Document *document, size_t tab_width)
{
    const DocumentLine *line;
    size_t visual_column;

    if (cursor == NULL || document == NULL || tab_width == 0 ||
        cursor->row >= document->line_count)
    {
        return false;
    }

    line = &document->lines[cursor->row];
    if (!text_layout_line_visual_column(line, cursor->column, tab_width, &visual_column))
    {
        return false;
    }

    cursor->preferred_visual_column = visual_column;
    cursor->preferred_visual_column_valid = true;
    return true;
}

/*
 * Move the cursor to `target_row` while preserving the preferred visual
 * column.
 *
 * If the preferred visual column has not been recorded yet (for example the
 * cursor was just placed programmatically), it is first derived from the
 * current position. The stored visual column is then mapped to the nearest
 * logical column on the destination line.
 *
 * Parameters:
 *   cursor     - the cursor to move.
 *   document   - the document the cursor refers to.
 *   target_row - the zero-based line to move to.
 *   tab_width  - the configured tab stop width; must be greater than 0.
 *
 * Returns:
 *   true if the cursor moved, false on NULL arguments, a tab width of 0, an
 *   out-of-range row, or an inconsistent line.
 */
static bool cursor_move_vertical(Cursor *cursor, const Document *document, size_t target_row, size_t tab_width)
{
    const DocumentLine *target_line;
    size_t target_column;

    if (cursor == NULL || document == NULL || tab_width == 0 || cursor->row >= document->line_count || target_row >= document->line_count)
    {
        return false;
    }

    if (!cursor->preferred_visual_column_valid && !cursor_update_preferred_visual_column(cursor, document, tab_width))
    {
        return false;
    }

    target_line = &document->lines[target_row];
    if (!text_layout_line_logical_column(target_line, cursor->preferred_visual_column, tab_width, &target_column))
    {
        return false;
    }

    cursor->row = target_row;
    cursor->column = target_column;
    return true;
}

/*
 * Move the cursor up one line, preserving the preferred visual column.
 *
 * Parameters:
 *   cursor   - the cursor to move.
 *   document - the document the cursor refers to.
 *
 * Returns:
 *   true if the cursor moved up, false on NULL arguments or if the cursor is
 *   already on the first line.
 */
bool cursor_move_up(Cursor *cursor, const Document *document, size_t tab_width)
{
    if (cursor == NULL || document == NULL || cursor->row == 0 ||
        cursor->row >= document->line_count)
    {
        return false;
    }

    return cursor_move_vertical(cursor, document, cursor->row - 1, tab_width);
}

/*
 * Move the cursor down one line, preserving the preferred visual column.
 *
 * Parameters:
 *   cursor   - the cursor to move.
 *   document - the document the cursor refers to.
 *
 * Returns:
 *   true if the cursor moved down, false on NULL arguments or if the cursor is
 *   already on the last line.
 */
bool cursor_move_down(Cursor *cursor, const Document *document, size_t tab_width)
{
    if (cursor == NULL || document == NULL ||
        cursor->row >= document->line_count ||
        cursor->row + 1 >= document->line_count)
    {
        return false;
    }

    return cursor_move_vertical(cursor, document, cursor->row + 1, tab_width);
}

/*
 * Move the cursor upward by one visible page while preserving its preferred
 * visual column.
 *
 * Parameters:
 *   cursor    - the cursor to move.
 *   document  - the document the cursor refers to.
 *   page_rows - the number of document rows in one visible page.
 *
 * Returns:
 *   true if the cursor position changed, false for invalid arguments or if
 *   the cursor is already at the first row.
 */
bool cursor_move_page_up(Cursor *cursor, const Document *document, size_t page_rows, size_t tab_width)
{
    size_t target_row;

    if (cursor == NULL || document == NULL || page_rows == 0 ||
        cursor->row >= document->line_count || cursor->row == 0)
    {
        return false;
    }

    target_row = cursor->row > page_rows ? cursor->row - page_rows : 0;
    if (target_row == cursor->row)
    {
        return false;
    }

    return cursor_move_vertical(cursor, document, target_row, tab_width);
}

/*
 * Move the cursor downward by one visible page while preserving its preferred
 * visual column.
 *
 * Parameters:
 *   cursor    - the cursor to move.
 *   document  - the document the cursor refers to.
 *   page_rows - the number of document rows in one visible page.
 *
 * Returns:
 *   true if the cursor position changed, false for invalid arguments or if
 *   the cursor is already at the last row.
 */
bool cursor_move_page_down(Cursor *cursor, const Document *document, size_t page_rows, size_t tab_width)
{
    size_t remaining_rows;
    size_t target_row;

    if (cursor == NULL || document == NULL || page_rows == 0 ||
        cursor->row >= document->line_count ||
        cursor->row + 1 >= document->line_count)
    {
        return false;
    }

    remaining_rows = document->line_count - 1 - cursor->row;
    if (page_rows >= remaining_rows)
    {
        target_row = document->line_count - 1;
    }
    else
    {
        target_row = cursor->row + page_rows;
    }

    return cursor_move_vertical(cursor, document, target_row, tab_width);
}
