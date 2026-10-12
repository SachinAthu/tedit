#include "viewport.h"

void viewport_init(Viewport *viewport)
{
    if (viewport == NULL)
    {
        return;
    }

    viewport->viewport_row = 0;
    viewport->viewport_column = 0;
}

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
