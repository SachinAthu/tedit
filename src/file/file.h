#ifndef FILE_FILE_H
#define FILE_FILE_H

#include "document/document.h"

typedef enum
{
    FILE_LOAD_OK,
    FILE_LOAD_INVALID_ARGUMENT,
    FILE_LOAD_OPEN_FAILED,
    FILE_LOAD_READ_FAILED,
    FILE_LOAD_INVALID_UTF8,
    FILE_LOAD_OUT_OF_MEMORY
} FileLoadResult;

/*
 * Load a UTF-8 file into a Document.
 *
 * On success, replace the contents of the output Document with the
 * decoded file contents. Recognize LF and CRLF line endings.
 *
 * On failure, leave the caller's existing Document unchanged.
 *
 * The caller owns the resulting Document and must eventually
 * release it with document_destroy().
 */
FileLoadResult file_load_document(const char *path, Document *out_document);

#endif