#define _POSIX_C_SOURCE 200809L

#include "file.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INITIAL_FILE_BUFFER_CAPACITY 4096

/*
 * A growable buffer holding the raw bytes of a file exactly as read from
 * disk, before any UTF-8 decoding.
 *
 * `length` is the number of bytes currently in use and `capacity` is how
 * many fit in `data`. The buffer is not NUL-terminated.
 */
typedef struct
{
    unsigned char *data;
    size_t length;
    size_t capacity;
} FileByteBuffer;

/*
 * Release a FileByteBuffer's storage and reset its fields so it can no
 * longer be used.
 */
static void file_byte_buffer_destroy(FileByteBuffer *buffer)
{
    if (buffer == NULL)
    {
        return;
    }

    free(buffer->data);
    buffer->data = NULL;
    buffer->length = 0;
    buffer->capacity = 0;
}

/*
 * Grow a FileByteBuffer so it can hold at least `required` bytes.
 *
 * The capacity starts at INITIAL_FILE_BUFFER_CAPACITY and doubles until it
 * reaches `required`. If doubling would overflow a size_t, the buffer is
 * resized to exactly `required` (and the following realloc() reports failure
 * if that still cannot be satisfied).
 *
 * Parameters:
 *   buffer   - the buffer to grow.
 *   required - the minimum number of bytes the buffer must hold.
 *
 * Returns:
 *   true if the buffer now holds at least `required` bytes, false if the
 *   reallocation failed (the buffer is left unchanged in that case).
 */
static bool file_byte_buffer_reserve(FileByteBuffer *buffer, size_t required)
{
    if (required <= buffer->capacity)
    {
        return true;
    }

    size_t new_capacity = buffer->capacity == 0 ? INITIAL_FILE_BUFFER_CAPACITY : buffer->capacity;

    while (new_capacity < required)
    {
        if (new_capacity > SIZE_MAX / 2)
        {
            new_capacity = required;
            break;
        }

        new_capacity *= 2;
    }

    unsigned char *new_data = realloc(buffer->data, new_capacity);

    if (new_data == NULL)
    {
        return false;
    }

    buffer->data = new_data;
    buffer->capacity = new_capacity;

    return true;
}

/*
 * Read an entire file descriptor into `buffer`, in fixed-size chunks.
 *
 * The loop appends each chunk to the buffer, growing it as needed, and stops
 * when read() returns zero (end of file). Interrupted reads (EINTR) are
 * retried; any other read error is reported.
 *
 * Parameters:
 *   descriptor - the open file descriptor to read.
 *   buffer     - the buffer that receives the file's bytes.
 *
 * Returns:
 *   FILE_LOAD_OK on success, FILE_LOAD_READ_FAILED on a read error, or
 *   FILE_LOAD_OUT_OF_MEMORY if the buffer cannot grow.
 */
static FileLoadResult file_read_all(int descriptor, FileByteBuffer *buffer)
{
    unsigned char chunk[4096];

    for (;;)
    {
        ssize_t bytes_read = read(descriptor, chunk, sizeof(chunk));

        if (bytes_read < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return FILE_LOAD_READ_FAILED;
        }

        if (bytes_read == 0)
        {
            return FILE_LOAD_OK;
        }

        if ((size_t)bytes_read > SIZE_MAX - buffer->length)
        {
            return FILE_LOAD_OUT_OF_MEMORY;
        }

        size_t required = buffer->length + (size_t)bytes_read;

        if (!file_byte_buffer_reserve(buffer, required))
        {
            return FILE_LOAD_OUT_OF_MEMORY;
        }

        memcpy(buffer->data + buffer->length, chunk, (size_t)bytes_read);

        buffer->length = required;
    }
}

/*
 * Decode a single UTF-8 sequence starting at `offset`.
 *
 * The lead byte selects the sequence length:
 *   0xxxxxxx                -> 1 byte  (ASCII)
 *   110xxxxx  (0xC2-0xDF)   -> 2 bytes
 *   1110xxxx  (0xE0-0xEF)   -> 3 bytes
 *   11110xxx  (0xF0-0xF4)   -> 4 bytes
 * Anything else (a stray continuation byte, an overlong 0xC0/0xC1 lead, or
 * a lead above 0xF4) is rejected as malformed.
 *
 * For multi-byte sequences the lead byte's low bits are the most significant
 * bits of the code point, and each continuation byte (10xxxxxx) contributes
 * six more bits. The extra checks on the first continuation byte reject bit
 * patterns that would otherwise decode to illegal values:
 *   - 0xE0 followed by < 0xA0 rejects overlong encodings below U+0800,
 *   - 0xED followed by > 0x9F rejects UTF-16 surrogates U+D800-U+DFFF,
 *   - 0xF0 followed by < 0x90 rejects overlong encodings below U+10000,
 *   - 0xF4 followed by > 0x8F rejects values above U+10FFFF.
 *
 * Parameters:
 *   bytes          - the buffer holding the file's raw bytes.
 *   length         - number of bytes in `bytes`.
 *   offset         - index of the first byte of the sequence.
 *   out_code_point - out-parameter: receives the decoded code point.
 *
 * Returns:
 *   the number of bytes consumed (1..4), or 0 if the input is malformed.
 *   `out_code_point` is written only on success.
 */
static size_t file_decode_utf8(const unsigned char *bytes, size_t length, size_t offset, uint32_t *out_code_point)
{
    unsigned char first;
    uint32_t code_point;
    size_t sequence_length;

    if (bytes == NULL || out_code_point == NULL || offset >= length)
    {
        return 0;
    }

    first = bytes[offset];

    if (first <= 0x7F)
    {
        *out_code_point = first;
        return 1;
    }

    if (first >= 0xC2 && first <= 0xDF)
    {
        sequence_length = 2;
        code_point = first & 0x1F;
    }
    else if (first >= 0xE0 && first <= 0xEF)
    {
        sequence_length = 3;
        code_point = first & 0x0F;
    }
    else if (first >= 0xF0 && first <= 0xF4)
    {
        sequence_length = 4;
        code_point = first & 0x07;
    }
    else
    {
        return 0;
    }

    if (sequence_length > length - offset)
    {
        return 0;
    }

    for (size_t index = 1; index < sequence_length; ++index)
    {
        unsigned char continuation = bytes[offset + index];

        if ((continuation & 0xC0) != 0x80)
        {
            return 0;
        }

        /*
         * Reject overlong encodings, UTF-16 surrogates, and values
         * above U+10FFFF through checks on the second byte.
         */
        if (index == 1)
        {
            if (first == 0xE0 && continuation < 0xA0)
            {
                return 0;
            }

            if (first == 0xED && continuation > 0x9F)
            {
                return 0;
            }

            if (first == 0xF0 && continuation < 0x90)
            {
                return 0;
            }

            if (first == 0xF4 && continuation > 0x8F)
            {
                return 0;
            }
        }

        code_point = (code_point << 6) | (continuation & 0x3F);
    }

    *out_code_point = code_point;

    return sequence_length;
}

/*
 * Strip a trailing carriage return from the document's last line, if any.
 *
 * CRLF line endings are handled during decoding: the CR is appended to the
 * current line like any other code point, and this helper removes it when the
 * LF that follows is seen. This makes CRLF and LF files decode identically.
 *
 * This is best-effort and cannot fail. If the document is NULL or has no
 * lines, or its last line is empty or does not end in CR, it simply does
 * nothing.
 */
static void file_remove_trailing_carriage_return(Document *document)
{
    DocumentLine *line;

    if (document == NULL || document->line_count == 0)
    {
        return;
    }

    line = document_get_line(document, document->line_count - 1);

    if (line == NULL || line->length == 0)
    {
        return;
    }

    if (line->data[line->length - 1] == UINT32_C(0x0D))
    {
        document_delete_character(document, document->line_count - 1, line->length - 1);
    }
}

/*
 * Decode the raw file bytes into `document`, one code point at a time.
 *
 * LF (U+000A) terminates the current logical line: it strips a preceding CR
 * and, unless it is the last byte of the file, splits off a new line for the
 * content that follows. Every other code point is appended to the current
 * line. An empty file leaves the document with zero lines.
 *
 * Parameters:
 *   bytes    - the raw file contents to decode.
 *   document - the target Document, already initialized with one blank line.
 *
 * Returns:
 *   FILE_LOAD_OK on success, FILE_LOAD_INVALID_UTF8 if any sequence is
 *   malformed, or FILE_LOAD_OUT_OF_MEMORY if a line operation fails.
 */
static FileLoadResult file_decode_document(const FileByteBuffer *bytes, Document *document)
{
    size_t offset = 0;

    /*
     * An empty file is represented by a zero-line Document.
     * A newly created, unsaved document still starts with one blank line.
     */
    if (bytes->length == 0)
    {
        document_destroy(document);
        return FILE_LOAD_OK;
    }

    while (offset < bytes->length)
    {
        uint32_t code_point;
        size_t consumed = file_decode_utf8(bytes->data, bytes->length, offset, &code_point);

        if (consumed == 0)
        {
            return FILE_LOAD_INVALID_UTF8;
        }

        offset += consumed;

        if (code_point == UINT32_C(0x0A))
        {
            file_remove_trailing_carriage_return(document);

            /*
             * A final line ending terminates the current logical line.
             * It does not create an additional line beyond that line.
             *
             * A non-final line ending splits the current line and creates
             * the next one.
             */
            if (offset < bytes->length)
            {
                DocumentLine *line = document_get_line(document, document->line_count - 1);

                if (line == NULL || !document_split_line(document, document->line_count - 1, line->length))
                {
                    return FILE_LOAD_OUT_OF_MEMORY;
                }
            }

            continue;
        }

        if (document->line_count == 0)
        {
            /*
             * Non-empty input always begins with one logical line.
             * This branch also keeps the decoder safe if its initialization
             * policy changes in a later implementation.
             */
            return FILE_LOAD_INVALID_ARGUMENT;
        }

        DocumentLine *line = document_get_line(document, document->line_count - 1);

        if (line == NULL || !document_insert_character(document, document->line_count - 1, line->length, code_point))
        {
            return FILE_LOAD_OUT_OF_MEMORY;
        }
    }

    return FILE_LOAD_OK;
}

/*
 * Load a UTF-8 file into a Document. See file.h for the full contract.
 *
 * The work is done in three phases so that the caller's existing document is
 * preserved on any failure:
 *   1. Read the whole file into a byte buffer.
 *   2. Decode those bytes into a temporary Document.
 *   3. Only on success, destroy the caller's document and move the temporary
 *      document's contents into it.
 */
FileLoadResult file_load_document(const char *path, Document *out_document)
{
    int descriptor;
    FileByteBuffer bytes = {0};
    Document loaded_document;
    FileLoadResult result;

    if (path == NULL || path[0] == '\0' || out_document == NULL)
    {
        return FILE_LOAD_INVALID_ARGUMENT;
    }

    descriptor = open(path, O_RDONLY);

    if (descriptor < 0)
    {
        return FILE_LOAD_OPEN_FAILED;
    }

    result = file_read_all(descriptor, &bytes);

    if (close(descriptor) != 0 && result == FILE_LOAD_OK)
    {
        result = FILE_LOAD_READ_FAILED;
    }

    if (result != FILE_LOAD_OK)
    {
        file_byte_buffer_destroy(&bytes);
        return result;
    }

    if (!document_init(&loaded_document))
    {
        file_byte_buffer_destroy(&bytes);
        return FILE_LOAD_OUT_OF_MEMORY;
    }

    result = file_decode_document(&bytes, &loaded_document);
    file_byte_buffer_destroy(&bytes);

    if (result != FILE_LOAD_OK)
    {
        document_destroy(&loaded_document);
        return result;
    }

    /*
     * Commit only after reading and decoding the complete file.
     * The caller's existing Document remains unchanged on failure.
     */
    document_destroy(out_document);
    *out_document = loaded_document;

    return FILE_LOAD_OK;
}
