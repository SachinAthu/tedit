#ifndef DOCUMENT_DOCUMENT_H
#define DOCUMENT_DOCUMENT_H

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    char *data;
    size_t length;
    size_t capacity;
} DocumentLine;

typedef struct
{
    DocumentLine *lines;
    size_t line_count;
    size_t line_capacity;
} Document;

bool document_init(Document *document);

void document_destroy(Document *document);

DocumentLine *document_get_line(Document *document, size_t row);

bool document_insert_character(Document *document, size_t row, size_t column, char character);

bool document_delete_character(Document *document, size_t row, size_t column);

bool document_split_line(Document *document, size_t row, size_t column);

bool document_merge_lines(Document *document, size_t row);

#endif