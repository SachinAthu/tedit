#ifndef DOCUMENT_DOCUMENT_H
#define DOCUMENT_DOCUMENT_H

#include <stdbool.h>
#include <stddef.h>

/* One line of text. The characters are stored in a buffer that grows as needed. */
typedef struct
{
    char *data;     /* The buffer holding this line's characters. */
    size_t length;  /* How many characters are currently stored. */
    size_t capacity;/* How many characters `data` can hold without growing. */
} DocumentLine;

/* The whole document: an ordered list of lines, plus bookkeeping for growth. */
typedef struct
{
    DocumentLine *lines;       /* Array of lines; lines[0] is the first line. */
    size_t line_count;         /* How many lines currently exist. */
    size_t line_capacity;      /* How many lines `lines` can hold without growing. */
} Document;

bool document_init(Document *document);

void document_destroy(Document *document);

DocumentLine *document_get_line(Document *document, size_t row);

bool document_insert_character(Document *document, size_t row, size_t column, char character);

bool document_delete_character(Document *document, size_t row, size_t column);

bool document_split_line(Document *document, size_t row, size_t column);

bool document_merge_lines(Document *document, size_t row);

#endif