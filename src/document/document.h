#ifndef DOCUMENT_DOCUMENT_H
#define DOCUMENT_DOCUMENT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * A logical line containing Unicode code points.
 *
 * length and capacity are measured in code points, not UTF-8 bytes.
 * The array is not NUL-terminated.
 */
typedef struct
{
    uint32_t *data;
    size_t length;
    size_t capacity;
} DocumentLine;

/*
 * A Document owns an ordered, dynamically sized collection of lines.
 */
typedef struct
{
    DocumentLine *lines;
    size_t line_count;
    size_t line_capacity;
} Document;

/*
 * Initialize a Document containing one empty logical line.
 *
 * Parameters:
 *   document - the document to initialize; must not be NULL.
 *
 * Returns:
 *   true on success, false if `document` is NULL or allocation failed.
 */
bool document_init(Document *document);

/*
 * Release all storage owned by a Document and reset it to a zero-line state.
 *
 * Parameters:
 *   document - the document to release; may be NULL.
 *
 * Returns:
 *   void.
 */
void document_destroy(Document *document);

/*
 * Return the line at zero-based `row`.
 *
 * The pointer is valid only until the document's line array is next grown or
 * modified.
 *
 * Parameters:
 *   document - the document to read from.
 *   row      - the zero-based line index.
 *
 * Returns:
 *   a pointer to the requested line, or NULL if `document` is NULL or `row`
 *   is out of range.
 */
DocumentLine *document_get_line(Document *document, size_t row);

/*
 * Insert one Unicode code point at (row, column), shifting the rest of the
 * line one position to the right. If the Document has zero lines (the
 * representation of an empty file), only position (0, 0) is valid and the
 * insertion creates the first line.
 *
 * Parameters:
 *   document   - the document to modify.
 *   row        - the zero-based line to insert into.
 *   column     - the zero-based insertion position (0 to line length).
 *   code_point - the Unicode scalar to insert; surrogates and values above
 *                U+10FFFF are rejected.
 *
 * Returns:
 *   true if the character was inserted, false on NULL arguments, invalid
 *   positions, an invalid code point, or allocation failure.
 */
bool document_insert_character(Document *document, size_t row, size_t column, uint32_t code_point);

/*
 * Delete the code point at (row, column), shifting the rest of the line one
 * position to the left.
 *
 * Parameters:
 *   document - the document to modify.
 *   row      - the zero-based line to delete from.
 *   column   - the zero-based position of the code point to delete.
 *
 * Returns:
 *   true if a code point was removed, false if `document` is NULL or the
 *   position is invalid (column past the end of the line).
 */
bool document_delete_character(Document *document, size_t row, size_t column);

/*
 * Split a line at (row, column): the suffix starting at `column` moves into a
 * new line inserted directly below, and the original line keeps only its
 * prefix. For a zero-line Document, splitting at (0, 0) creates two empty
 * logical lines.
 *
 * Parameters:
 *   document - the document to modify.
 *   row      - the zero-based line to split.
 *   column   - the zero-based split position (0 to line length).
 *
 * Returns:
 *   true on success, false on NULL arguments, invalid positions, or
 *   allocation failure.
 */
bool document_split_line(Document *document, size_t row, size_t column);

/*
 * Merge the line at `row` with the line immediately below it: the lower
 * line's contents are appended to the upper line and the lower line is
 * removed.
 *
 * Parameters:
 *   document - the document to modify.
 *   row      - the zero-based index of the upper line.
 *
 * Returns:
 *   true on success, false if `document` is NULL, `row` is the last line, or
 *   allocation failed.
 */
bool document_merge_lines(Document *document, size_t row);

#endif