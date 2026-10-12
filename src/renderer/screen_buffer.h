#ifndef RENDERER_SCREEN_BUFFER_H
#define RENDERER_SCREEN_BUFFER_H

#include <stdbool.h>
#include <stddef.h>

/* One cell on the screen. For now it stores just a single character; in the
 * future it may grow to hold color and other style information. */
typedef struct
{
    char character;
} ScreenCell;

/* The off-screen grid the renderer draws into before sending it to the
 * terminal. Cells are stored in row-major order in a single flat array. */
typedef struct
{
    ScreenCell *cells; /* Flat array of rows * columns cells. */
    size_t rows;       /* Number of rows in the grid. */
    size_t columns;    /* Number of columns in the grid. */
} ScreenBuffer;

bool screen_buffer_init(ScreenBuffer *buffer, size_t rows, size_t columns);

void screen_buffer_destroy(ScreenBuffer *buffer);

void screen_buffer_clear(ScreenBuffer *buffer, char character);

bool screen_buffer_set(ScreenBuffer *buffer, size_t row, size_t column, char character);

const ScreenCell *screen_buffer_get(const ScreenBuffer *buffer, size_t row, size_t column);

#endif