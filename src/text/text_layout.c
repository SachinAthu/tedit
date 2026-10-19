#define _XOPEN_SOURCE 700
#include "text_layout.h"

#include <wchar.h>

/*
 * Return whether `value` lies within the inclusive range [first, last].
 *
 * This tiny helper keeps the Unicode range tables in
 * text_layout_code_point_width() readable: instead of spelling out
 * `value >= 0x0300 && value <= 0x036F` many times, each entry reads as a
 * single "is this in the range ...?" check.
 *
 * Parameters:
 *   value - the value to test.
 *   first - the lower bound of the range (inclusive).
 *   last  - the upper bound of the range (inclusive).
 *
 * Returns:
 *   true if first <= value <= last, false otherwise.
 */
static bool text_layout_in_range(uint32_t value, uint32_t first, uint32_t last)
{
    return value >= first && value <= last;
}

/*
 * Compute how many terminal columns `code_point` advances the visual cursor,
 * writing the result through `advance`.
 *
 * The three cases mirror the renderer's on-screen behavior:
 *   - a tab jumps to the next configured tab stop, which depends on the
 *     current visual column;
 *   - an unprintable/control character still occupies one column (it is
 *     rendered as '?');
 *   - a leading combining mark (a zero-width character at the very start of a
 *     line) has nothing to combine with, so it is shown on a dotted circle
 *     and therefore occupies one column too.
 *
 * Parameters:
 *   code_point    - the Unicode scalar to measure.
 *   visual_column - the current zero-based visual column.
 *   tab_width     - the configured tab stop width; must be greater than 0.
 *   advance       - out-parameter: receives the number of columns to advance.
 *
 * Returns:
 *   always true; `advance` is always written.
 */
static bool text_layout_advance(uint32_t code_point, size_t visual_column, size_t tab_width, size_t *advance)
{
    int width;

    if (code_point == (uint32_t)'\t')
    {
        *advance = tab_width - (visual_column % tab_width);
        return true;
    }

    width = text_layout_code_point_width(code_point);

    if (width < 0)
    {
        width = 1;
    }

    if (width == 0 && visual_column == 0)
    {
        /* Match rendering of a leading combining mark with a dotted circle. */
        width = 1;
    }

    *advance = (size_t)width;

    return true;
}

/*
 * Return the terminal display width of one Unicode code point.
 *
 * The system wcwidth() is tried first; it is accurate in a UTF-8-aware
 * locale. When it returns -1 - which it does in the POSIX/C locale, where
 * the library knows nothing about non-ASCII characters - the hard-coded
 * Unicode tables below provide a deterministic answer.
 *
 * Parameters:
 *   code_point - the Unicode scalar to measure.
 *
 * Returns:
 *   2 for a wide (double-width) character, 1 for a narrow character, 0 for a
 *   zero-width combining mark, or -1 for an invalid value (a UTF-16
 *   surrogate or a value above U+10FFFF).
 */
int text_layout_code_point_width(uint32_t code_point)
{
    int width;

    if (code_point > UINT32_C(0x10FFFF) || text_layout_in_range(code_point, UINT32_C(0xD800), UINT32_C(0xDFFF)))
    {
        return -1;
    }

    width = wcwidth((wchar_t)code_point);

    if (width >= 0)
    {
        return width;
    }

    /* Deterministic fallback when the active locale lacks Unicode widths. */
    if (text_layout_in_range(code_point, 0x0300, 0x036F) ||
        text_layout_in_range(code_point, 0x0483, 0x0489) ||
        text_layout_in_range(code_point, 0x0591, 0x05BD) ||
        text_layout_in_range(code_point, 0x05BF, 0x05BF) ||
        text_layout_in_range(code_point, 0x05C1, 0x05C2) ||
        text_layout_in_range(code_point, 0x0610, 0x061A) ||
        text_layout_in_range(code_point, 0x064B, 0x065F) ||
        text_layout_in_range(code_point, 0x0670, 0x0670) ||
        text_layout_in_range(code_point, 0x06D6, 0x06ED) ||
        text_layout_in_range(code_point, 0x1AB0, 0x1AFF) ||
        text_layout_in_range(code_point, 0x1DC0, 0x1DFF) ||
        text_layout_in_range(code_point, 0x20D0, 0x20FF) ||
        text_layout_in_range(code_point, 0xFE00, 0xFE0F) ||
        text_layout_in_range(code_point, 0xFE20, 0xFE2F) ||
        text_layout_in_range(code_point, 0xE0100, 0xE01EF))
    {
        return 0;
    }

    if (text_layout_in_range(code_point, 0x1100, 0x115F) ||
        text_layout_in_range(code_point, 0x2329, 0x232A) ||
        text_layout_in_range(code_point, 0x2E80, 0xA4CF) ||
        text_layout_in_range(code_point, 0xAC00, 0xD7A3) ||
        text_layout_in_range(code_point, 0xF900, 0xFAFF) ||
        text_layout_in_range(code_point, 0xFE10, 0xFE19) ||
        text_layout_in_range(code_point, 0xFE30, 0xFE6F) ||
        text_layout_in_range(code_point, 0xFF00, 0xFF60) ||
        text_layout_in_range(code_point, 0xFFE0, 0xFFE6) ||
        text_layout_in_range(code_point, 0x1F300, 0x1FAFF) ||
        text_layout_in_range(code_point, 0x20000, 0x3FFFD))
    {
        return 2;
    }

    return 1;
}

/*
 * Convert a logical code-point boundary to its zero-based visual column.
 *
 * The first `logical_column` code points of the line are walked and their
 * display widths summed, so tabs and wide characters are reflected in the
 * result. This is the inverse of text_layout_line_logical_column().
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
bool text_layout_line_visual_column(const DocumentLine *line, size_t logical_column, size_t tab_width, size_t *visual_column)
{
    size_t visual = 0;

    if (line == NULL || visual_column == NULL || tab_width == 0 || logical_column > line->length || (line->length > 0 && line->data == NULL))
    {
        return false;
    }

    for (size_t index = 0; index < logical_column; ++index)
    {
        size_t advance;

        if (!text_layout_advance(line->data[index], visual, tab_width, &advance) || visual > SIZE_MAX - advance)
        {
            return false;
        }
        visual += advance;
    }

    *visual_column = visual;

    return true;
}

/*
 * Convert a visual column to the nearest logical code-point boundary.
 *
 * When the target visual column falls inside a tab or a wide character, the
 * boundary nearest to it is chosen; on a tie the boundary before that
 * character is preferred. This lets vertical cursor movement land cleanly at
 * the start or end of a tab stop or double-width character rather than
 * leaving the cursor "inside" one.
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
bool text_layout_line_logical_column(const DocumentLine *line, size_t target_visual_column, size_t tab_width, size_t *logical_column)
{
    size_t visual = 0;

    if (line == NULL || logical_column == NULL || tab_width == 0 || (line->length > 0 && line->data == NULL))
    {
        return false;
    }

    for (size_t index = 0; index < line->length; ++index)
    {
        size_t advance;
        size_t next_visual;

        if (target_visual_column <= visual)
        {
            *logical_column = index;
            return true;
        }

        if (!text_layout_advance(line->data[index], visual, tab_width, &advance) || visual > SIZE_MAX - advance)
        {
            return false;
        }

        next_visual = visual + advance;

        if (target_visual_column < next_visual)
        {
            size_t distance_before = target_visual_column - visual;
            size_t distance_after = next_visual - target_visual_column;
            *logical_column = distance_before <= distance_after ? index : index + 1;

            return true;
        }

        visual = next_visual;
    }

    *logical_column = line->length;

    return true;
}
