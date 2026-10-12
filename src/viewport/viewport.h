#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    size_t viewport_row;
    size_t viewport_column;
} Viewport;

void viewport_init(Viewport *viewport);

bool viewport_ensure_cursor_visible(Viewport *viewport, size_t cursor_row, size_t cursor_visual_column, size_t screen_rows, size_t screen_columns);

#endif