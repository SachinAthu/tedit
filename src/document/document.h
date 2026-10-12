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
 */
bool document_init(Document *document);

/*
 * Release all storage owned by a Document.
 */
void document_destroy(Document *document);

/*
 * Return a line by zero-based row, or NULL if the row is invalid.
 */
DocumentLine *document_get_line(Document *document, size_t row);

/*
 * Insert one Unicode code point at column.
 */
bool document_insert_character(Document *document, size_t row, size_t column, uint32_t code_point);

/*
 * Delete the code point at column.
 */
bool document_delete_character(Document *document, size_t row, size_t column);

/*
 * Split a line at column, moving its suffix into a new line.
 */
bool document_split_line(Document *document, size_t row, size_t column);

/*
 * Merge the line at row with the line immediately below it.
 */
bool document_merge_lines(Document *document, size_t row);

#endif