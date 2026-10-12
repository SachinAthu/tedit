#ifndef RENDERER_RENDERER_H
#define RENDERER_RENDERER_H

#include <stdbool.h>
#include <stddef.h>

#include "state/editor_state.h"
#include "screen_buffer.h"

/* Owns everything needed to draw a frame: the off-screen grid we draw into,
 * plus a reusable output buffer for the escape sequences sent to the terminal. */
typedef struct
{
    ScreenBuffer screen_buffer; /* Off-screen grid of the visible document. */
    char *output_buffer;        /* Reused buffer holding the terminal output. */
    size_t output_capacity;     /* How many bytes `output_buffer` can hold. */
} Renderer;

bool renderer_init(Renderer *renderer, size_t rows, size_t columns);

void renderer_destroy(Renderer *renderer);

bool renderer_render(Renderer *renderer, const EditorState *state);

#endif