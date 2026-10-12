#ifndef RENDERER_RENDERER_H
#define RENDERER_RENDERER_H

#include <stdbool.h>
#include <stddef.h>

#include "state/editor_state.h"
#include "screen_buffer.h"

/*
 * Everything needed to draw one frame of the editor to the terminal.
 *
 *   - screen_buffer: the off-screen grid of characters we draw into before
 *                    sending the whole frame to the terminal.
 *   - output_buffer: a reusable byte buffer that holds the frame (escape
 *                    sequences plus characters) about to be written out.
 *   - output_capacity: how many bytes output_buffer currently holds.
 */
typedef struct
{
    ScreenBuffer screen_buffer;
    char *output_buffer;
    size_t output_capacity;
} Renderer;

/*
 * Prepare a Renderer for use, allocating its screen buffer.
 *
 * Parameters:
 *   renderer - the renderer to initialize; must not be NULL.
 *   rows     - number of text rows the screen buffer should hold.
 *   columns  - number of text columns the screen buffer should hold.
 *
 * Returns:
 *   true on success, false if `renderer` is NULL or the buffer could not be
 *   allocated.
 */
bool renderer_init(Renderer *renderer, size_t rows, size_t columns);

/*
 * Release everything the renderer allocated.
 *
 * Parameters:
 *   renderer - the renderer to clean up; may be NULL.
 */
void renderer_destroy(Renderer *renderer);

/*
 * Draw one complete frame of the editor: the visible document plus the cursor.
 *
 * Parameters:
 *   renderer - the renderer that owns the off-screen buffer.
 *   state    - the current editor state to draw (read-only).
 *
 * Returns:
 *   true on success, false on NULL arguments or if any drawing step fails.
 */
bool renderer_render(Renderer *renderer, const EditorState *state);

#endif