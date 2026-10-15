#define _POSIX_C_SOURCE 200809L

#include "file/file.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FILE_SAVE_BUFFER_CAPACITY 4096
#define TEMPORARY_PATH_SUFFIX ".tedit-XXXXXX"

/*
 * A small write buffer that batches encoded bytes before they go to disk.
 *
 * Writing byte by byte would issue one write() per code point; buffering
 * here means the file is written in a handful of 4 KiB chunks instead.
 *
 *   - descriptor: the open file the bytes are flushed to.
 *   - data:       the pending bytes not yet flushed.
 *   - length:     how many bytes in `data` are currently in use.
 */
typedef struct
{
    int descriptor;
    unsigned char data[FILE_SAVE_BUFFER_CAPACITY];
    size_t length;
} FileOutputBuffer;

/*
 * Write exactly `length` bytes to the file, retrying as needed.
 *
 * Parameters:
 *   descriptor - the open file descriptor to write to.
 *   data       - the bytes to write.
 *   length     - how many bytes to write.
 *
 * Returns:
 *   true if every byte was written, false on a write error (other than an
 *   interruption, which is retried).
 */
static bool file_write_all(int descriptor, const unsigned char *data, size_t length)
{
    size_t offset = 0;

    while (offset < length)
    {
        ssize_t bytes_written = write(descriptor, data + offset, length - offset);

        if (bytes_written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return false;
        }

        if (bytes_written == 0)
        {
            return false;
        }

        offset += (size_t)bytes_written;
    }

    return true;
}

/*
 * Write any pending buffered bytes to disk and mark the buffer empty.
 *
 * Parameters:
 *   buffer - the buffer to flush.
 *
 * Returns:
 *   true if the flush succeeded (or the buffer was already empty), false on
 *   a write error. On failure the pending bytes stay in the buffer.
 */
static bool file_output_flush(FileOutputBuffer *buffer)
{
    if (buffer->length == 0)
    {
        return true;
    }

    if (!file_write_all(buffer->descriptor, buffer->data, buffer->length))
    {
        return false;
    }

    buffer->length = 0;
    return true;
}

/*
 * Append one raw byte to the buffer, flushing to disk when full.
 *
 * Parameters:
 *   buffer - the buffer to append to.
 *   byte   - the byte to append.
 *
 * Returns:
 *   true on success, false if the buffer is full and flushing failed.
 */
static bool file_output_byte(FileOutputBuffer *buffer, unsigned char byte)
{
    if (buffer->length == sizeof(buffer->data) && !file_output_flush(buffer))
    {
        return false;
    }

    buffer->data[buffer->length++] = byte;
    return true;
}

/*
 * Encode one Unicode scalar as UTF-8 and append the resulting bytes to the
 * buffer.
 *
 * Parameters:
 *   buffer     - the buffer to append to.
 *   code_point - the scalar to encode; values above U+10FFFF are invalid.
 *
 * Returns:
 *   true on success, false if `code_point` is not a valid scalar or writing
 *   failed.
 */
static bool file_output_code_point(FileOutputBuffer *buffer, uint32_t code_point)
{
    if (code_point <= UINT32_C(0x7F))
    {
        return file_output_byte(buffer, (unsigned char)code_point);
    }

    if (code_point <= UINT32_C(0x7FF))
    {
        return file_output_byte(buffer, (unsigned char)(0xC0 | (code_point >> 6))) &&
               file_output_byte(buffer, (unsigned char)(0x80 | (code_point & 0x3F)));
    }

    if (code_point <= UINT32_C(0xFFFF))
    {
        return file_output_byte(buffer, (unsigned char)(0xE0 | (code_point >> 12))) &&
               file_output_byte(buffer, (unsigned char)(0x80 | ((code_point >> 6) & 0x3F))) &&
               file_output_byte(buffer, (unsigned char)(0x80 | (code_point & 0x3F)));
    }

    if (code_point <= UINT32_C(0x10FFFF))
    {
        return file_output_byte(buffer, (unsigned char)(0xF0 | (code_point >> 18))) &&
               file_output_byte(buffer, (unsigned char)(0x80 | ((code_point >> 12) & 0x3F))) &&
               file_output_byte(buffer, (unsigned char)(0x80 | ((code_point >> 6) & 0x3F))) &&
               file_output_byte(buffer, (unsigned char)(0x80 | (code_point & 0x3F)));
    }

    return false;
}

/*
 * Serialize an entire Document to the given file descriptor as UTF-8.
 *
 * Every line's code points are encoded, then the line is terminated with LF
 * (U+000A), so a zero-line Document writes nothing at all.
 *
 * Parameters:
 *   descriptor - the open file descriptor to write to.
 *   document   - the Document to serialize.
 *
 * Returns:
 *   FILE_SAVE_OK on success, FILE_SAVE_INVALID_ARGUMENT if the document is
 *   inconsistent, FILE_SAVE_INVALID_CODE_POINT if a line holds a value that
 *   is not a valid Unicode scalar, or FILE_SAVE_WRITE_FAILED on a write
 *   error.
 */
static FileSaveResult file_write_document(int descriptor, const Document *document)
{
    FileOutputBuffer output = {.descriptor = descriptor, .length = 0};

    for (size_t row = 0; row < document->line_count; ++row)
    {
        const DocumentLine *line = &document->lines[row];

        if (line->length > 0 && line->data == NULL)
        {
            return FILE_SAVE_INVALID_ARGUMENT;
        }

        for (size_t column = 0; column < line->length; ++column)
        {
            uint32_t code_point = line->data[column];

            if ((code_point >= UINT32_C(0xD800) && code_point <= UINT32_C(0xDFFF)) || code_point > UINT32_C(0x10FFFF))
            {
                return FILE_SAVE_INVALID_CODE_POINT;
            }

            if (!file_output_code_point(&output, code_point))
            {
                return FILE_SAVE_WRITE_FAILED;
            }
        }

        if (!file_output_byte(&output, (unsigned char)'\n'))
        {
            return FILE_SAVE_WRITE_FAILED;
        }
    }

    return file_output_flush(&output) ? FILE_SAVE_OK : FILE_SAVE_WRITE_FAILED;
}

/*
 * Save a Document to `path` atomically. See file.h for the full contract.
 *
 * The document is written to a uniquely named temporary file in the same
 * directory, flushed and closed, and only then renamed over the destination
 * - so a crash or error part-way through never corrupts the existing file.
 *
 * Parameters:
 *   path     - destination path; must not be NULL or empty.
 *   document - the Document to serialize; must not be NULL.
 *
 * Returns:
 *   a FileSaveResult describing the outcome. On any failure the temporary
 *   file is removed again.
 */
FileSaveResult file_save_document(const char *path, const Document *document)
{
    static const char temporary_suffix[] = TEMPORARY_PATH_SUFFIX;

    size_t path_length;
    char *temporary_path;
    int descriptor;
    FileSaveResult result;
    bool close_succeeded;

    if (path == NULL || path[0] == '\0' || document == NULL || (document->line_count > 0 && document->lines == NULL))
    {
        return FILE_SAVE_INVALID_ARGUMENT;
    }

    path_length = strlen(path);

    if (path_length > SIZE_MAX - sizeof(temporary_suffix))
    {
        return FILE_SAVE_OUT_OF_MEMORY;
    }

    temporary_path = malloc(path_length + sizeof(temporary_suffix));

    if (temporary_path == NULL)
    {
        return FILE_SAVE_OUT_OF_MEMORY;
    }

    memcpy(temporary_path, path, path_length);
    memcpy(temporary_path + path_length, temporary_suffix, sizeof(temporary_suffix));

    descriptor = mkstemp(temporary_path);

    if (descriptor < 0)
    {
        free(temporary_path);
        return FILE_SAVE_TEMPORARY_CREATE_FAILED;
    }

    result = file_write_document(descriptor, document);
    close_succeeded = close(descriptor) == 0;

    if (result == FILE_SAVE_OK && !close_succeeded)
    {
        result = FILE_SAVE_CLOSE_FAILED;
    }

    if (result == FILE_SAVE_OK && rename(temporary_path, path) != 0)
    {
        result = FILE_SAVE_REPLACE_FAILED;
    }

    if (result != FILE_SAVE_OK)
    {
        (void)unlink(temporary_path);
    }

    free(temporary_path);
    return result;
}