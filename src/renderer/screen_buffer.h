#ifndef RENDERER_SCREEN_BUFFER_H
#define RENDERER_SCREEN_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * A terminal display cell. Wide code points use a base cell plus a continuation
 * cell; zero-width combining code points are stored with their base cell.
 */
typedef struct
{
    uint32_t code_point;
    bool continuation;
    uint32_t *combining_code_points;
    size_t combining_count;
    size_t combining_capacity;
} ScreenCell;

/*
 * A two-dimensional grid of ScreenCells representing the visible screen.
 *
 * `cells` is a flat array laid out row by row (row-major order). `rows` and
 * `columns` give the grid's dimensions.
 */
typedef struct
{
    ScreenCell *cells;
    size_t rows;
    size_t columns;
} ScreenBuffer;

/*
 * Allocate a screen buffer of the given size and fill it with spaces.
 *
 * Parameters:
 *   buffer  - the buffer to initialize; must not be NULL.
 *   rows    - number of text rows; must be greater than 0.
 *   columns - number of text columns; must be greater than 0.
 *
 * Returns:
 *   true on success, false if the arguments are invalid or allocation fails.
 */
bool screen_buffer_init(ScreenBuffer *buffer, size_t rows, size_t columns);

/*
 * Free the buffer's cell storage and reset its dimensions to zero.
 *
 * Parameters:
 *   buffer - the buffer to release; may be NULL.
 */
void screen_buffer_destroy(ScreenBuffer *buffer);

/*
 * Overwrite every cell in the buffer with the given code point.
 *
 * Parameters:
 *   buffer     - the buffer to clear; may be NULL.
 *   code_point - the value to write into every cell.
 *
 * Returns:
 *   void.
 */
void screen_buffer_clear(ScreenBuffer *buffer, uint32_t code_point);

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
bool screen_buffer_set(ScreenBuffer *buffer, size_t row, size_t column, uint32_t code_point);

/*
 * Read the cell at a given position.
 *
 * Parameters:
 *   buffer - the buffer to read from.
 *   row    - the zero-based row to read.
 *   column - the zero-based column to read.
 *
 * Returns:
 *   a pointer to the cell, or NULL if `buffer` is NULL/unallocated or the
 *   coordinates are out of range.
 */
const ScreenCell *screen_buffer_get(const ScreenBuffer *buffer, size_t row, size_t column);

/* Mark a cell as the second column occupied by a wide code point. */
bool screen_buffer_set_continuation(ScreenBuffer *buffer, size_t row, size_t column);

/* Append a zero-width combining code point to an existing base cell. */
bool screen_buffer_append_combining(ScreenBuffer *buffer, size_t row, size_t column, uint32_t code_point);

#endif