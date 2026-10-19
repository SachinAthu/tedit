#ifndef TEXT_TEXT_LAYOUT_H
#define TEXT_TEXT_LAYOUT_H

#include "document/document.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Return the terminal display width of one Unicode code point. */
int text_layout_code_point_width(uint32_t code_point);

/* Convert a logical code-point boundary to its zero-based visual column. */
bool text_layout_line_visual_column(const DocumentLine *line, size_t logical_column, size_t tab_width, size_t *visual_column);

/*
 * Convert a visual column to the nearest logical code-point boundary.
 * When the target lies inside a tab or wide character, the nearest boundary
 * is selected; ties choose the boundary before that character.
 */
bool text_layout_line_logical_column(const DocumentLine *line, size_t visual_column, size_t tab_width, size_t *logical_column);

#endif
