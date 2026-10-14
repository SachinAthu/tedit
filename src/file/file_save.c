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

typedef struct
{
    int descriptor;
    unsigned char data[FILE_SAVE_BUFFER_CAPACITY];
    size_t length;
} FileOutputBuffer;

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

static bool file_output_byte(FileOutputBuffer *buffer, unsigned char byte)
{
    if (buffer->length == sizeof(buffer->data) && !file_output_flush(buffer))
    {
        return false;
    }

    buffer->data[buffer->length++] = byte;
    return true;
}

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