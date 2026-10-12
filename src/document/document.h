#ifndef DOCUMENT_DOCUMENT_H
#define DOCUMENT_DOCUMENT_H

#include <stdbool.h>
#include <stddef.h>

/*
 * A single line of text in the document.
 *
 * The characters are stored in a heap-allocated buffer `data` that can be
 * larger than the text it currently holds. `length` counts how many characters
 * are actually in use, while `capacity` counts how many fit in `data`. The
 * extra room lets us insert characters without reallocating every time.
 */
typedef struct
{
    char *data;
    size_t length;
    size_t capacity;
} DocumentLine;

/*
 * A document: an ordered, dynamically sized list of lines.
 *
 * `lines` points to an array of DocumentLine values. As with a line, the array
 * may be bigger than the number of lines in use: `line_count` is the real
 * number of lines, and `line_capacity` is how many slots are allocated.
 */
typedef struct
{
    DocumentLine *lines;
    size_t line_count;
    size_t line_capacity;
} Document;

/*
 * Create an empty document containing a single blank line.
 *
 * Parameters:
 *   document - the document to initialize.
 *
 * Returns:
 *   true on success, false if `document` is NULL or allocation fails.
 */
bool document_init(Document *document);

/*
 * Release all memory owned by a document and reset it to an empty state.
 *
 * Parameters:
 *   document - the document to destroy; may be NULL.
 */
void document_destroy(Document *document);

/*
 * Look up a line by its zero-based row index.
 *
 * Parameters:
 *   document - the document to read from.
 *   row      - the zero-based index of the line to fetch.
 *
 * Returns:
 *   a pointer to the requested line, or NULL if `document` is NULL or `row` is
 *   out of range.
 */
DocumentLine *document_get_line(Document *document, size_t row);

/*
 * Insert one character into a line, shifting the rest of the line right.
 *
 * Parameters:
 *   document  - the document to modify.
 *   row       - the zero-based index of the target line.
 *   column    - the zero-based position to insert at (0 to line length).
 *   character - the character to insert.
 *
 * Returns:
 *   true on success, false if the line does not exist, `column` is past the
 *   end of the line, or growing the line's buffer fails.
 */
bool document_insert_character(Document *document, size_t row, size_t column, char character);

/*
 * Delete one character from a line, shifting the rest of the line left.
 *
 * Parameters:
 *   document - the document to modify.
 *   row      - the zero-based index of the target line.
 *   column   - the zero-based position of the character to delete.
 *
 * Returns:
 *   true on success, false if the line does not exist or `column` does not
 *   point at a character.
 */
bool document_delete_character(Document *document, size_t row, size_t column);

/*
 * Split a line in two at a given column, turning it into two lines.
 *
 * The characters before `column` stay in the original line; the characters at
 * and after `column` move to a new line inserted directly below it.
 *
 * Parameters:
 *   document - the document to modify.
 *   row      - the zero-based index of the line to split.
 *   column   - the zero-based position to split at (0 to line length).
 *
 * Returns:
 *   true on success, false if the line does not exist, `column` is out of
 *   range, or allocation fails.
 */
bool document_split_line(Document *document, size_t row, size_t column);

/*
 * Join a line with the line directly below it.
 *
 * The second line's characters are appended to the end of the first line, then
 * the second line is removed.
 *
 * Parameters:
 *   document - the document to modify.
 *   row      - the zero-based index of the first line to merge.
 *
 * Returns:
 *   true on success, false if there is no line below `row`, or allocation
 *   fails.
 */
bool document_merge_lines(Document *document, size_t row);

#endif