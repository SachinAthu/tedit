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

typedef enum
{
    FILE_SAVE_OK,
    FILE_SAVE_INVALID_ARGUMENT,
    FILE_SAVE_TEMPORARY_CREATE_FAILED,
    FILE_SAVE_INVALID_CODE_POINT,
    FILE_SAVE_WRITE_FAILED,
    FILE_SAVE_CLOSE_FAILED,
    FILE_SAVE_REPLACE_FAILED,
    FILE_SAVE_OUT_OF_MEMORY
} FileSaveResult;

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

/*
 * Save the complete Document as UTF-8 with canonical LF line endings.
 * Each logical line receives a trailing LF. A zero-line Document
 * produces an empty file.
 *
 * The destination is replaced atomically after the temporary file
 * has been written and closed successfully.
 *
 * The caller retains ownership of the Document. Editor State and
 * its Dirty Flag are not modified by this function.
 */
FileSaveResult file_save_document(const char *path, const Document *document);

#endif