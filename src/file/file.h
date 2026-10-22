#ifndef FILE_FILE_H
#define FILE_FILE_H

#include "document/document.h"

/*
 * Outcome of file_load_document().
 *
 * FILE_LOAD_OK means the document was replaced with the decoded file
 * contents. Every other value reports why loading failed, so callers can
 * produce a useful message; the destination document is left untouched on
 * all failures.
 */
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
 * Outcome of file_save_document().
 *
 * FILE_SAVE_OK means the destination file was replaced with the serialized
 * document. Every other value reports which step failed; on all failures the
 * destination file keeps its previous contents (when a replacement is even
 * reached) and the caller's Document is unchanged.
 */
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
 * decoded file contents. Recognize LF and CRLF line endings. An empty
 * file produces a zero-line Document.
 *
 * On failure, leave the caller's existing Document unchanged.
 *
 * Parameters:
 *   path         - path of the file to read; must not be NULL or empty.
 *   out_document - the Document that receives the decoded contents; must
 *                  not be NULL.
 *
 * Returns:
 *   a FileLoadResult describing the outcome. The caller owns the resulting
 *   Document and must eventually release it with document_destroy().
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
 * Parameters:
 *   path     - destination path; must not be NULL or empty.
 *   document - the Document to serialize; must not be NULL.
 *
 * Returns:
 *   a FileSaveResult describing the outcome. The caller retains ownership
 *   of the Document; the Editor State's Dirty Flag is not modified by this
 *   function.
 */
FileSaveResult file_save_document(const char *path, const Document *document);

#endif
