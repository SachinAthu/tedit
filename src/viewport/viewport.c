#include "viewport.h"

/*
 * Reset the viewport so the document starts at the top-left (offset 0, 0).
 *
 * Parameters:
 *   viewport - the viewport to initialize; may be NULL.
 *
 * Returns:
 *   void.
 */
void viewport_init(Viewport *viewport)
{
    if (viewport == NULL)
    {
        return;
    }

    viewport->viewport_row = 0;
    viewport->viewport_column = 0;
}

/*
 * Scroll the viewport (if needed) so the cursor's position is on screen.
 *
 * If the cursor has moved above the visible area, the viewport scrolls up to
 * match it; if it has moved below, the viewport scrolls down just enough to
 * show it. The same logic applies horizontally. The viewport is never moved
 * more than necessary.
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
bool viewport_ensure_cursor_visible(Viewport *viewport, size_t cursor_row, size_t cursor_visual_column, size_t screen_rows, size_t screen_columns)
{
    if (viewport == NULL || screen_rows == 0 || screen_columns == 0)
    {
        return false;
    }

    /* If the cursor is above the top visible row, scroll up to it. */
    if (cursor_row < viewport->viewport_row)
    {
        viewport->viewport_row = cursor_row;
    }
    /* If it is at or below the bottom edge, scroll down to reveal it. The
     * "+1" places the cursor on the last visible row. */
    else if (cursor_row - viewport->viewport_row >= screen_rows)
    {
        viewport->viewport_row = cursor_row - screen_rows + 1;
    }

    /* The same idea applied horizontally. */
    if (cursor_visual_column < viewport->viewport_column)
    {
        viewport->viewport_column = cursor_visual_column;
    }
    else if (cursor_visual_column - viewport->viewport_column >= screen_columns)
    {
        viewport->viewport_column = cursor_visual_column - screen_columns + 1;
    }

    return true;
}
