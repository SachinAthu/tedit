#ifndef TEXT_TEXT_LAYOUT_H
#define TEXT_TEXT_LAYOUT_H

#include "document/document.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Return the terminal display width of one Unicode code point.
 *
 * Parameters:
 *   code_point - the Unicode scalar to measure.
 *
 * Returns:
 *   2 for a wide character, 1 for a narrow character, 0 for a zero-width
 *   combining mark, or -1 for an invalid value (a UTF-16 surrogate or a
 *   value above U+10FFFF).
 */
int text_layout_code_point_width(uint32_t code_point);

/*
 * Convert a logical code-point boundary to its zero-based visual column.
 *
 * Parameters:
 *   line           - the line to measure; must not be NULL.
 *   logical_column - the zero-based code-point boundary (0 to line length).
 *   tab_width      - the configured tab stop width; must be greater than 0.
 *   visual_column  - out-parameter: receives the resulting visual column.
 *
 * Returns:
 *   true on success, false on NULL arguments, a tab width of 0, a logical
 *   column past the end of the line, or an inconsistent line.
 */
bool text_layout_line_visual_column(const DocumentLine *line, size_t logical_column, size_t tab_width, size_t *visual_column);

/*
 * Convert a visual column to the nearest logical code-point boundary.
 * When the target lies inside a tab or wide character, the nearest boundary
 * is selected; ties choose the boundary before that character.
 *
 * Parameters:
 *   line                 - the line to map onto; must not be NULL.
 *   target_visual_column - the zero-based visual column to convert.
 *   tab_width            - the configured tab stop width; must be greater
 *                          than 0.
 *   logical_column       - out-parameter: receives the code-point boundary
 *                          (0 to line length).
 *
 * Returns:
 *   true on success, false on NULL arguments, a tab width of 0, or an
 *   inconsistent line.
 */
bool text_layout_line_logical_column(const DocumentLine *line, size_t target_visual_column, size_t tab_width, size_t *logical_column);

#endif
