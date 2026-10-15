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
 *   - document_rows: number of rows available to document text; the two
 *                    rows below it are reserved for the Status Bar and Message Bar.
 *   - output_buffer: a reusable byte buffer that holds the frame (escape
 *                    sequences plus characters) about to be written out.
 *   - output_capacity: how many bytes output_buffer currently holds.
 */
typedef struct
{
    ScreenBuffer screen_buffer;
    size_t document_rows;
    char *output_buffer;
    size_t output_capacity;
} Renderer;

bool renderer_init(Renderer *renderer, size_t rows, size_t columns);

void renderer_destroy(Renderer *renderer);

bool renderer_render(Renderer *renderer, const EditorState *state);

#endif