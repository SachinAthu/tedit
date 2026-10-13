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
 *   - preferred_column: the column the user last chose horizontally. It is
 *                       remembered so that moving up or down across lines of
 *                       different lengths keeps the cursor at a sensible spot.
 */
typedef struct
{
    size_t row;
    size_t column;
    size_t preferred_column;
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
 * Move the cursor up one line, keeping the preferred column when possible.
 *
 * Parameters:
 *   cursor   - the cursor to move.
 *   document - the document the cursor refers to.
 *
 * Returns:
 *   true if the cursor moved up, false if it is already on the first line.
 */
bool cursor_move_up(Cursor *cursor, const Document *document);

/*
 * Move the cursor down one line, keeping the preferred column when possible.
 *
 * Parameters:
 *   cursor   - the cursor to move.
 *   document - the document the cursor refers to.
 *
 * Returns:
 *   true if the cursor moved down, false if it is already on the last line.
 */
bool cursor_move_down(Cursor *cursor, const Document *document);

#endif