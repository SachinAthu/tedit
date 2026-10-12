#include "viewport.h"

/*
 * Reset the viewport's scroll position to the top-left corner (row 0, column 0).
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
 * Adjust the viewport's scroll offset so the cursor stays inside the visible
 * screen area.
 *
 * The viewport stores the row and column of the screen's top-left corner. If
 * the cursor is above or to the left of that corner, the offset moves back;
 * if it has scrolled past the bottom or right edge, the offset moves forward
 * so the cursor is the last visible cell. This keeps the cursor on screen
 * without scrolling more than necessary.
 *
 * Parameters:
 *   viewport             - the viewport whose scroll offset to adjust.
 *   cursor_row           - the zero-based row the cursor is on.
 *   cursor_visual_column - the zero-based column the cursor is on.
 *   screen_rows          - how many rows the screen can show (must be > 0).
 *   screen_columns       - how many columns the screen can show (must be > 0).
 *
 * Returns:
 *   true if the viewport was adjusted, false if `viewport` is NULL or either
 *   screen dimension is zero.
 */
bool viewport_ensure_cursor_visible(Viewport *viewport, size_t cursor_row, size_t cursor_visual_column, size_t screen_rows, size_t screen_columns)
{
    if (viewport == NULL || screen_rows == 0 || screen_columns == 0)
    {
        return false;
    }

    if (cursor_row < viewport->viewport_row)
    {
        viewport->viewport_row = cursor_row;
    }
    else if (cursor_row - viewport->viewport_row >= screen_rows)
    {
        viewport->viewport_row = cursor_row - screen_rows + 1;
    }

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
