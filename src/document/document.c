#include "document.h"

#include <stdlib.h>
#include <string.h>

/*
 * A document is a dynamic list of lines. Each line owns a dynamically sized
 * character buffer. Buffers grow on demand so callers can insert text without
 * worrying about fixed sizes.
 */

/* Starting size (in bytes) of a new line's character buffer. */
#define INITIAL_LINE_CAPACITY 16
/* Starting size (in lines) of a new document's array of lines. */
#define INITIAL_DOCUMENT_CAPACITY 8

/*
 * Prepare a single line for use: allocate its character buffer and set its
 * bookkeeping fields.
 *
 * Parameters:
 *   line - the line to initialize.
 *
 * Returns:
 *   true on success, false if `line` is NULL or allocation fails.
 */
static bool document_line_init(DocumentLine *line)
{
    if (line == NULL)
    {
        return false;
    }

    /* Allocate the initial storage that will hold the line's characters. */
    line->data = malloc(INITIAL_LINE_CAPACITY);

    if (line->data == NULL)
    {
        return false;
    }

    /* A new line is empty but has room for capacity characters. */
    line->length = 0;
    line->capacity = INITIAL_LINE_CAPACITY;

    return true;
}

/*
 * Release a single line's character buffer and reset its fields so the line
 * can no longer be used accidentally.
 *
 * Parameters:
 *   line - the line to release; may be NULL.
 *
 * Returns:
 *   void.
 */
static void document_line_destroy(DocumentLine *line)
{
    if (line == NULL)
    {
        return;
    }

    /* Give the buffer back to the system and clear the pointers/counters. */
    free(line->data);
    line->data = NULL;
    line->length = 0;
    line->capacity = 0;
}

/*
 * Make room in a line's buffer by doubling its capacity. Called when an insert
 * would otherwise exceed the current capacity.
 *
 * Parameters:
 *   line - the line whose character buffer should be grown.
 *
 * Returns:
 *   true on success, false if reallocation fails.
 */
static bool document_grow_line(DocumentLine *line)
{
    char *new_data;
    size_t new_capacity;

    /* A never-initialized line starts at the default capacity; others double. */
    if (line->capacity == 0)
    {
        new_capacity = INITIAL_LINE_CAPACITY;
    }
    else
    {
        new_capacity = line->capacity * 2;
    }

    /* Reallocate the existing buffer to the larger size. */
    new_data = realloc(line->data, new_capacity);

    if (new_data == NULL)
    {
        return false;
    }

    /* realloc may move the block, so store the new pointer and capacity. */
    line->data = new_data;
    line->capacity = new_capacity;

    return true;
}

/*
 * Make room in a document's array of lines by doubling its capacity. Called
 * when adding lines would otherwise exceed the current capacity.
 *
 * Parameters:
 *   document - the document whose line array should be grown.
 *
 * Returns:
 *   true on success, false if reallocation fails.
 */
static bool document_grow_lines(Document *document)
{
    DocumentLine *new_lines;
    size_t new_capacity;

    /* First allocation uses the default size; later growth doubles. */
    if (document->line_capacity == 0)
    {
        new_capacity = INITIAL_LINE_CAPACITY;
    }
    else
    {
        new_capacity = document->line_capacity * 2;
    }

    /* Resize the array; each slot holds one DocumentLine. */
    new_lines = realloc(document->lines, new_capacity * sizeof(*new_lines));

    if (new_lines == NULL)
    {
        return false;
    }

    document->lines = new_lines;
    document->line_capacity = new_capacity;

    return true;
}

/*
 * Create an empty document containing a single blank line.
 *
 * Parameters:
 *   document - the document to initialize.
 *
 * Returns:
 *   true on success, false if `document` is NULL or any allocation fails.
 */
bool document_init(Document *document)
{
    if (document == NULL)
    {
        return false;
    }

    /* Start with no lines and no storage; grow_lines will allocate the array. */
    document->lines = NULL;
    document->line_count = 0;
    document->line_capacity = 0;

    /* Allocate room for at least one line. */
    if (!document_grow_lines(document))
    {
        return false;
    }

    /*
     * Initialize the first line slot so the document always starts with one
     * usable (blank) line, like a real text editor.
     */
    if (!document_line_init(&document->lines[0]))
    {
        /* Clean up the line array we just allocated before giving up. */
        free(document->lines);

        document->lines = NULL;
        document->line_capacity = 0;

        return false;
    }

    /* Exactly one line now exists. */
    document->line_count = 1;

    return true;
}

/*
 * Free every line's buffer plus the array that holds them, then reset the
 * document to an empty state. Safe to call on a NULL document.
 *
 * Parameters:
 *   document - the document to destroy; may be NULL.
 *
 * Returns:
 *   void.
 */
void document_destroy(Document *document)
{
    if (document == NULL)
    {
        return;
    }

    /* Release each line's character buffer. */
    for (size_t index = 0; index < document->line_count; index++)
    {
        document_line_destroy(&document->lines[index]);
    }

    /* Release the array of lines itself and reset bookkeeping. */
    free(document->lines);
    document->lines = NULL;
    document->line_count = 0;
    document->line_capacity = 0;
}

/*
 * Look up a line by its row index.
 *
 * Parameters:
 *   document - the document to read from.
 *   row      - the zero-based index of the line to fetch.
 *
 * Returns:
 *   a pointer to the line at `row`, or NULL if `document` is NULL or `row` is
 *   out of range.
 */
DocumentLine *document_get_line(Document *document, size_t row)
{
    if (document == NULL || row >= document->line_count)
    {
        return NULL;
    }

    return &document->lines[row];
}

/*
 * Insert one character into the given line at the given column, shifting the
 * remaining characters to the right.
 *
 * Parameters:
 *   document  - the document to modify.
 *   row       - the zero-based index of the target line.
 *   column    - the zero-based position to insert at (0 to length).
 *   character - the character to insert.
 *
 * Returns:
 *   true on success, false if the line does not exist, `column` is past the
 *   end of the line, or growing the line fails.
 */
bool document_insert_character(Document *document, size_t row, size_t column, char character)
{
    DocumentLine *line = document_get_line(document, row);

    /* Reject invalid rows and columns past the end of the line. */
    if (line == NULL || column > line->length)
    {
        return false;
    }

    /* Grow the buffer first if inserting would fill or exceed capacity. */
    if (line->length + 1 >= line->capacity)
    {
        if (!document_grow_line(line))
        {
            return false;
        }
    }

    /*
     * Shift everything from the insertion point one byte to the right, then
     * drop the new character into the freed slot and bump the length.
     */
    memmove(&line->data[column + 1], &line->data[column], line->length - column);
    line->data[column] = character;
    line->length += 1;

    return true;
}

/*
 * Delete one character from the given line at the given column, shifting the
 * remaining characters to the left.
 *
 * Parameters:
 *   document - the document to modify.
 *   row      - the zero-based index of the target line.
 *   column   - the zero-based position of the character to delete.
 *
 * Returns:
 *   true on success, false if the line does not exist or `column` is at or past
 *   the end of the line.
 */
bool document_delete_character(Document *document, size_t row, size_t column)
{
    DocumentLine *line = document_get_line(document, row);

    /* Reject invalid rows and columns that do not point at a character. */
    if (line == NULL || column >= line->length)
    {
        return false;
    }

    /*
     * Shift everything after the deleted character one byte to the left, which
     * overwrites it, then shrink the length.
     */
    memmove(&line->data[column], &line->data[column + 1], line->length - column - 1);
    line->length -= 1;

    return true;
}