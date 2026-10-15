#include "screen_buffer.h"

#include <stdint.h>
#include <stdlib.h>

/*
 * Allocate and clear a screen buffer of the given size. The buffer stores one
 * ScreenCell per position in a row-major grid, initially filled with spaces.
 *
 * Parameters:
 *   buffer  - the screen buffer to initialize.
 *   rows    - number of text rows; must be greater than 0.
 *   columns - number of text columns; must be greater than 0.
 *
 * Returns:
 *   true on success, false if `buffer` is NULL, either size is 0, the size
 *   overflows, or allocation fails.
 */
bool screen_buffer_init(ScreenBuffer *buffer, size_t rows, size_t columns)
{
    if (buffer == NULL || rows == 0 || columns == 0)
    {
        return false;
    }

    /* Start from a known-empty state before allocating. */
    buffer->cells = NULL;
    buffer->rows = 0;
    buffer->columns = 0;

    /* Reject sizes where the cell count itself would overflow a size_t. */
    if (rows > SIZE_MAX / columns)
    {
        return false;
    }

    size_t cell_count = rows * columns;

    /* Reject sizes whose byte size would overflow a size_t. */
    if (cell_count > SIZE_MAX / sizeof(*buffer->cells))
    {
        return false;
    }

    /* Allocate one contiguous block holding every cell. */
    buffer->cells = malloc(cell_count * sizeof(*buffer->cells));
    if (buffer->cells == NULL)
    {
        return false;
    }

    buffer->rows = rows;
    buffer->columns = columns;

    /* Begin with a blank screen. */
    screen_buffer_clear(buffer, ' ');

    return true;
}

/*
 * Free the buffer's cell storage and reset its dimensions to zero.
 *
 * Parameters:
 *   buffer - the screen buffer to release; may be NULL.
 *
 * Returns:
 *   void.
 */
void screen_buffer_destroy(ScreenBuffer *buffer)
{
    if (buffer == NULL)
    {
        return;
    }

    free(buffer->cells);

    buffer->cells = NULL;
    buffer->rows = 0;
    buffer->columns = 0;
}

/*
 * Overwrite every cell in the buffer with the given code point.
 *
 * Used at the start of each frame so characters left over from the previous
 * frame do not show through.
 *
 * Parameters:
 *   buffer     - the buffer to clear; may be NULL.
 *   code_point - the value to write into every cell.
 *
 * Returns:
 *   void.
 */
void screen_buffer_clear(ScreenBuffer *buffer, uint32_t code_point)
{
    if (buffer == NULL || buffer->cells == NULL)
    {
        return;
    }

    size_t cell_count = buffer->rows * buffer->columns;

    /* Overwrite each cell; only the code_point field is used for now. */
    for (size_t index = 0; index < cell_count; ++index)
    {
        buffer->cells[index].code_point = code_point;
    }
}

/*
 * Write one code point into the cell at (row, column).
 *
 * Parameters:
 *   buffer     - the buffer to write into.
 *   row        - the zero-based row to write.
 *   column     - the zero-based column to write.
 *   code_point - the value to store in the cell.
 *
 * Returns:
 *   true on success, false if `buffer` is NULL/unallocated or the
 *   coordinates are out of range.
 */
bool screen_buffer_set(ScreenBuffer *buffer, size_t row, size_t column, uint32_t code_point)
{
    size_t index;

    if (buffer == NULL || buffer->cells == NULL || row >= buffer->rows || column >= buffer->columns)
    {
        return false;
    }

    /* Convert the 2D coordinate into a 1D block index. */
    index = row * buffer->columns + column;
    buffer->cells[index].code_point = code_point;

    return true;
}

/*
 * Read the cell at a given position.
 *
 * Parameters:
 *   buffer - the screen buffer to read from.
 *   row    - the zero-based row to read.
 *   column - the zero-based column to read.
 *
 * Returns:
 *   a pointer to the cell at (row, column), or NULL if `buffer` is NULL or
 *   unallocated, or the coordinates are out of range.
 */
const ScreenCell *screen_buffer_get(const ScreenBuffer *buffer, size_t row, size_t column)
{
    size_t index;

    if (buffer == NULL || buffer->cells == NULL || row >= buffer->rows || column >= buffer->columns)
    {
        return NULL;
    }

    index = row * buffer->columns + column;

    return &buffer->cells[index];
}
