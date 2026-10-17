#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Which part of the document is currently visible on screen.
 *
 * A document can be much larger than the terminal, so the editor shows only a
 * "window" into it. `viewport_row` is the first visible logical line, and
 * `viewport_column` is the first visible visual display column after tab
 * expansion. Both coordinates are zero-based.
 */
typedef struct
{
    size_t viewport_row;
    size_t viewport_column;
} Viewport;

/*
 * Reset the viewport so the document starts at the top-left (offset 0, 0).
 *
 * Parameters:
 *   viewport - the viewport to initialize; may be NULL.
 */
void viewport_init(Viewport *viewport);

/*
 * Scroll the viewport (if needed) so the cursor's position is on screen.
 *
 * The viewport is only moved enough to bring the cursor back into view; if the
 * cursor is already visible, the viewport is left unchanged.
 *
 * Parameters:
 *   viewport             - the viewport to adjust.
 *   cursor_row           - the cursor's zero-based line.
 *   cursor_visual_column - the cursor's zero-based column.
 *   screen_rows          - how many lines the screen can show.
 *   screen_columns       - how many characters per line the screen can show.
 *
 * Returns:
 *   true on success, false if `viewport` is NULL or either screen dimension is
 *   zero.
 */
bool viewport_ensure_cursor_visible(Viewport *viewport, size_t cursor_row, size_t cursor_visual_column, size_t screen_rows, size_t screen_columns);

#endif
