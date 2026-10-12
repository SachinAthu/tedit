#include "document.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_LINE_CAPACITY 16
#define INITIAL_DOCUMENT_CAPACITY 8

static bool document_line_init(DocumentLine *line)
{
    if (line == NULL)
    {
        return false;
    }

    line->data = malloc(INITIAL_LINE_CAPACITY * sizeof(*line->data));
    if (line->data == NULL)
    {
        return false;
    }

    line->length = 0;
    line->capacity = INITIAL_LINE_CAPACITY;

    return true;
}

static void document_line_destroy(DocumentLine *line)
{
    if (line == NULL)
    {
        return;
    }

    free(line->data);
    line->data = NULL;
    line->length = 0;
    line->capacity = 0;
}

static bool document_grow_line(DocumentLine *line)
{
    size_t new_capacity;
    uint32_t *new_data;

    if (line == NULL)
    {
        return false;
    }

    if (line->capacity == 0)
    {
        new_capacity = INITIAL_LINE_CAPACITY;
    }
    else
    {
        if (line->capacity > SIZE_MAX / 2)
        {
            return false;
        }

        new_capacity = line->capacity * 2;
    }

    if (new_capacity > SIZE_MAX / sizeof(*line->data))
    {
        return false;
    }

    new_data = realloc(line->data, new_capacity * sizeof(*line->data));

    if (new_data == NULL)
    {
        return false;
    }

    line->data = new_data;
    line->capacity = new_capacity;

    return true;
}

static bool document_grow_lines(Document *document)
{
    size_t new_capacity;
    DocumentLine *new_lines;

    if (document == NULL)
    {
        return false;
    }

    if (document->line_capacity == 0)
    {
        new_capacity = INITIAL_DOCUMENT_CAPACITY;
    }
    else
    {
        if (document->line_capacity > SIZE_MAX / 2)
        {
            return false;
        }

        new_capacity = document->line_capacity * 2;
    }

    if (new_capacity > SIZE_MAX / sizeof(*document->lines))
    {
        return false;
    }

    new_lines = realloc(document->lines, new_capacity * sizeof(*document->lines));

    if (new_lines == NULL)
    {
        return false;
    }

    document->lines = new_lines;
    document->line_capacity = new_capacity;

    return true;
}

bool document_init(Document *document)
{
    if (document == NULL)
    {
        return false;
    }

    document->lines = NULL;
    document->line_count = 0;
    document->line_capacity = 0;

    if (!document_grow_lines(document))
    {
        return false;
    }

    if (!document_line_init(&document->lines[0]))
    {
        free(document->lines);
        document->lines = NULL;
        document->line_capacity = 0;
        return false;
    }

    document->line_count = 1;

    return true;
}

void document_destroy(Document *document)
{
    if (document == NULL)
    {
        return;
    }

    for (size_t index = 0; index < document->line_count; ++index)
    {
        document_line_destroy(&document->lines[index]);
    }

    free(document->lines);

    document->lines = NULL;
    document->line_count = 0;
    document->line_capacity = 0;
}

DocumentLine *document_get_line(Document *document, size_t row)
{
    if (document == NULL || row >= document->line_count)
    {
        return NULL;
    }

    return &document->lines[row];
}

bool document_insert_character(Document *document, size_t row, size_t column, uint32_t code_point)
{
    DocumentLine *line = document_get_line(document, row);

    if (line == NULL || column > line->length)
    {
        return false;
    }

    /*
     * Reject values outside the Unicode code-point range and UTF-16
     * surrogate values. UTF-8 validation at the file boundary will
     * enforce the same restriction for loaded content.
     */
    if (code_point > UINT32_C(0x10FFFF) || (code_point >= UINT32_C(0xD800) && code_point <= UINT32_C(0xDFFF)))
    {
        return false;
    }

    if (line->length == SIZE_MAX)
    {
        return false;
    }

    if (line->length == line->capacity && !document_grow_line(line))
    {
        return false;
    }

    memmove(&line->data[column + 1], &line->data[column], (line->length - column) * sizeof(*line->data));

    line->data[column] = code_point;
    ++line->length;

    return true;
}

bool document_delete_character(Document *document, size_t row, size_t column)
{
    DocumentLine *line = document_get_line(document, row);

    if (line == NULL || column >= line->length)
    {
        return false;
    }

    memmove(&line->data[column], &line->data[column + 1], (line->length - column - 1) * sizeof(*line->data));

    --line->length;

    return true;
}

bool document_split_line(Document *document, size_t row, size_t column)
{
    DocumentLine *line = document_get_line(document, row);
    DocumentLine new_line;
    size_t suffix_length;
    size_t new_capacity;

    if (line == NULL || column > line->length)
    {
        return false;
    }

    suffix_length = line->length - column;
    new_capacity = suffix_length > INITIAL_LINE_CAPACITY ? suffix_length : INITIAL_LINE_CAPACITY;

    if (new_capacity > SIZE_MAX / sizeof(*new_line.data))
    {
        return false;
    }

    new_line.data = malloc(new_capacity * sizeof(*new_line.data));
    if (new_line.data == NULL)
    {
        return false;
    }

    new_line.length = suffix_length;
    new_line.capacity = new_capacity;

    if (suffix_length > 0)
    {
        memcpy(new_line.data, &line->data[column], suffix_length * sizeof(*new_line.data));
    }

    /*
     * Reserve the new line slot before modifying the existing line.
     */
    if (document->line_count == document->line_capacity && !document_grow_lines(document))
    {
        document_line_destroy(&new_line);
        return false;
    }

    /*
     * Reacquire the pointer: growing the line array may have moved it.
     */
    line = &document->lines[row];

    memmove(&document->lines[row + 2], &document->lines[row + 1], (document->line_count - row - 1) * sizeof(*document->lines));

    document->lines[row + 1] = new_line;
    ++document->line_count;
    line->length = column;

    return true;
}

bool document_merge_lines(Document *document, size_t row)
{
    DocumentLine *first_line;
    DocumentLine *second_line;
    size_t combined_length;

    if (document == NULL || row + 1 >= document->line_count)
    {
        return false;
    }

    first_line = &document->lines[row];
    second_line = &document->lines[row + 1];

    if (second_line->length > SIZE_MAX - first_line->length)
    {
        return false;
    }

    combined_length = first_line->length + second_line->length;

    while (first_line->capacity < combined_length)
    {
        if (!document_grow_line(first_line))
        {
            return false;
        }
    }

    if (second_line->length > 0)
    {
        memcpy(&first_line->data[first_line->length], second_line->data, second_line->length * sizeof(*second_line->data));
    }

    first_line->length = combined_length;
    document_line_destroy(second_line);

    memmove(&document->lines[row + 1], &document->lines[row + 2], (document->line_count - row - 2) * sizeof(*document->lines));

    --document->line_count;

    return true;
}