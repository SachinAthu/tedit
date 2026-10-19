#ifndef CURSOR_CURSOR_H
#define CURSOR_CURSOR_H

#include "document/document.h"

#include <stdbool.h>
#include <stddef.h>

/*
 * The position of the text cursor within a document.
 *
 *   - row:             which line the cursor is on (zero-based).
 *   - column:          which code point within that line (zero-based).
 *   - preferred_column: the logical column most recently chosen horizontally.
 *   - preferred_visual_column: the display column preserved during vertical
 *                               movement across tabs and Unicode widths.
 *   - preferred_visual_column_valid: whether the visual preference matches the
 *                                    current logical cursor position.
 */
typedef struct
{
    size_t row;
    size_t column;
    size_t preferred_column;
    size_t preferred_visual_column;
    bool preferred_visual_column_valid;
} Cursor;

/*
 * Place the cursor at the start of the document (row 0, column 0).
 *
 * Parameters:
 *   cursor - the cursor to initialize; may be NULL.
 */
void cursor_init(Cursor *cursor);

/*
 * Move the cursor to an explicit position inside the document.
 *
 * Parameters:
 *   cursor   - the cursor to move.
 *   document - the document the cursor refers to.
 *   row      - the zero-based target line index.
 *   column   - the zero-based target column (0 to line length).
 *
 * Returns:
 *   true if the cursor moved, false if `cursor` or `document` is NULL, `row`
 *   is out of range, or `column` is past the end of the line.
 */
bool cursor_set_position(Cursor *cursor, const Document *document, size_t row, size_t column);

/*
 * Move the cursor one column to the right within the current line.
 *
 * Parameters:
 *   cursor   - the cursor to move.
 *   document - the document the cursor refers to.
 *
 * Returns:
 *   true if the cursor advanced, false on NULL arguments or if the cursor is
 *   already at the end of the line.
 */
bool cursor_advance(Cursor *cursor, const Document *document);

/*
 * Move the cursor one column to the left within the current line.
 *
 * Parameters:
 *   cursor - the cursor to move.
 *
 * Returns:
 *   true if the cursor moved back, false if `cursor` is NULL or the cursor is
 *   already at the start of the line.
 */
bool cursor_move_left(Cursor *cursor);

/*
 * Move the cursor one column to the right, without passing the end of the
 * current line.
 *
 * Parameters:
 *   cursor   - the cursor to move.
 *   document - the document the cursor refers to.
 *
 * Returns:
 *   true if the cursor advanced, false on NULL arguments or if the cursor is
 *   already at the end of the line.
 */
bool cursor_move_right(Cursor *cursor, const Document *document);

/*
 * Move the cursor to the beginning of its current logical line.
 *
 * Parameters:
 *   cursor - the cursor to move.
 *
 * Returns:
 *   true if the cursor moved, false when cursor is NULL or already at column 0.
 */
bool cursor_move_home(Cursor *cursor);

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
bool cursor_move_end(Cursor *cursor, const Document *document);

/* Recompute the remembered visual column from the current logical position. */
bool cursor_update_preferred_visual_column(Cursor *cursor, const Document *document, size_t tab_width);

/*
 * Move the cursor up one line, preserving the preferred visual column.
 *
 * Parameters:
 *   cursor   - the cursor to move.
 *   document - the document the cursor refers to.
 *
 * Returns:
 *   true if the cursor moved up, false if it is already on the first line.
 */
bool cursor_move_up(Cursor *cursor, const Document *document, size_t tab_width);

/*
 * Move the cursor up by a page-sized number of logical lines while preserving
 * the preferred visual column.
 *
 * Parameters:
 *   cursor    - the cursor to move.
 *   document  - the document the cursor refers to.
 *   page_rows - the number of document rows in one visible page; must be > 0.
 *
 * Returns:
 *   true if the cursor position changed, false for invalid arguments or when
 *   the cursor is already at the top boundary.
 */
bool cursor_move_page_up(Cursor *cursor, const Document *document, size_t page_rows, size_t tab_width);

/*
 * Move the cursor down by a page-sized number of logical lines while preserving
 * the preferred visual column.
 *
 * Parameters:
 *   cursor    - the cursor to move.
 *   document  - the document the cursor refers to.
 *   page_rows - the number of document rows in one visible page; must be > 0.
 *
 * Returns:
 *   true if the cursor position changed, false for invalid arguments or when
 *   the cursor is already at the bottom boundary.
 */
bool cursor_move_page_down(Cursor *cursor, const Document *document, size_t page_rows, size_t tab_width);

/*
 * Move the cursor down one line, preserving the preferred visual column.
 *
 * Parameters:
 *   cursor   - the cursor to move.
 *   document - the document the cursor refers to.
 *
 * Returns:
 *   true if the cursor moved down, false if it is already on the last line.
 */
bool cursor_move_down(Cursor *cursor, const Document *document, size_t tab_width);

#endif