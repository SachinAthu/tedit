#ifndef RENDERER_SCREEN_BUFFER_H
#define RENDERER_SCREEN_BUFFER_H

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    char character;
} ScreenCell;

typedef struct
{
    ScreenCell *cells;
    size_t rows;
    size_t columns;
} ScreenBuffer;

bool screen_buffer_init(ScreenBuffer *buffer, size_t rows, size_t columns);

void screen_buffer_destroy(ScreenBuffer *buffer);

void screen_buffer_clear(ScreenBuffer *buffer, char character);

bool screen_buffer_set(ScreenBuffer *buffer, size_t row, size_t column, char character);

const ScreenCell *screen_buffer_get(const ScreenBuffer *buffer, size_t row, size_t column);

#endif