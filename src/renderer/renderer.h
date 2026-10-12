#ifndef RENDERER_RENDERER_H
#define RENDERER_RENDERER_H

#include <stdbool.h>
#include <stddef.h>

#include "state/editor_state.h"
#include "screen_buffer.h"

typedef struct
{
    ScreenBuffer screen_buffer;
    char *output_buffer;
    size_t output_capacity;
} Renderer;

bool renderer_init(Renderer *renderer, size_t rows, size_t columns);

void renderer_destroy(Renderer *renderer);

bool renderer_render(Renderer *renderer, const EditorState *state);

#endif