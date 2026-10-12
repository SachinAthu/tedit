#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <stdbool.h>
#include <stddef.h>

/* The scroll position of the screen: the document row/column shown at the
 * screen's top-left corner. Used to keep the cursor within the visible area. */
typedef struct
{
    size_t viewport_row;    /* First document row shown on screen. */
    size_t viewport_column; /* First document column shown on screen. */
} Viewport;

void viewport_init(Viewport *viewport);

bool viewport_ensure_cursor_visible(Viewport *viewport, size_t cursor_row, size_t cursor_visual_column, size_t screen_rows, size_t screen_columns);

#endif