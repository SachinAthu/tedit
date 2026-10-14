# Implementation Status

## Project Status

**Active — Volume X implementation**

Chapters 45–50 are implemented in the current source snapshot. Chapter 51 now covers UTF-8 serialization, safe temporary-file replacement, Editor State Dirty Flag handling, Ctrl-S integration for the startup filename, and failure-path regression checks. A clean CMake build and all four registered CTest targets (`smoke`, `editing`, `file`, and `file_save`) pass.

## Completed

### Volume X Preparation

- [x] Implementation repository and project structure established
- [x] Git workflow established
- [x] CMake build structure and C99 configuration established
- [x] Development and release build configurations defined
- [x] Testing baseline established
- [x] Implementation documentation initialized

### Chapter 45 — Establishing the Runtime

- [x] Terminal state and initialization implemented
- [x] Original terminal attributes preserved
- [x] Raw terminal mode implemented
- [x] Terminal restoration implemented
- [x] Application lifecycle and blocking event loop implemented
- [x] Temporary `Ctrl-Q` exit and EOF handling implemented
- [x] Failure-path cleanup integrated
- [x] Terminal module added to the build
- [x] Existing smoke test retained
- [x] Runtime milestone committed to Git

### Chapter 46 — Building the First Editable Screen

- [x] `EditorState` implemented as the central editor state
- [x] Dynamic `Document` with a vector of lines implemented
- [x] Basic `Cursor` state and position validation implemented
- [x] Character insertion into document lines implemented
- [x] Reusable contiguous `ScreenBuffer` implemented
- [x] Full-redraw `Renderer` implemented
- [x] Terminal output buffered to reduce output operations
- [x] Terminal text cursor hidden during rendering and restored afterward
- [x] Initial screen display implemented
- [x] Printable ASCII insertion and Cursor advancement integrated
- [x] Event loop integrated with the editing and rendering path
- [x] Cleanup for Renderer, Editor State, and terminal state integrated

### Chapter 47 — Editing Text

- [x] Left and Right Arrow input recognized for horizontal Cursor movement
- [x] Cursor movement constrained to the current logical line
- [x] Backspace deletes the character immediately before the Cursor when the Cursor column is greater than zero
- [x] Cursor moves to the deleted character's former position after Backspace
- [x] Document character deletion operation implemented with row/column validation
- [x] Dirty Flag added to `EditorState` and initialized to `false`
- [x] Successful character insertion marks the Document dirty
- [x] Successful Backspace deletion marks the Document dirty
- [x] Editing changes trigger a screen redraw
- [x] Smoke and editing automated tests pass in the Chapter 47 repository snapshot

### Chapter 48 — Working with Lines

- [x] `document_split_line()` splits a logical line at a valid row and column
- [x] Splitting at the beginning or end of a line supports empty logical lines
- [x] `document_merge_lines()` appends the following line and removes it from the Document
- [x] Enter and line feed trigger line splitting at the Cursor
- [x] Backspace at column zero merges with the previous line when one exists
- [x] Cursor moves to the beginning of the new line after splitting
- [x] Cursor moves to the previous line's former end after merging
- [x] Backspace at the beginning of the first line leaves the Document unchanged
- [x] Successful line edits update the Dirty Flag and trigger a redraw
- [x] Editing tests cover splitting, merging, empty lines, invalid operations, and Cursor transitions
- [x] User confirmed the editor builds and works in the terminal and all tests pass

### Chapter 49 — Navigating the Document

- [x] Cursor vertical movement implemented for Up and Down
- [x] Preferred-column behavior implemented and preserved across short or empty lines
- [x] Horizontal movement and editing position changes keep preferred-column state consistent
- [x] Viewport state added with vertical and horizontal offsets
- [x] Cursor visibility maintained as the Viewport scrolls
- [x] Renderer draws the visible document region using Viewport offsets
- [x] Up and Down Arrow input integrated into the event loop
- [x] Renderer initialization reserves the terminal's final physical column to avoid right-edge wrap behavior
- [x] Regression tests cover navigation, Viewport boundaries, editing transitions, and rendering with offsets
- [x] User confirmed vertical scrolling works after the fix
- [x] User confirmed horizontal typing no longer closes the editor in WSL after reserving the final terminal column

### Chapter 50 — Opening Files

- [x] File Manager reads complete files through POSIX APIs.
- [x] UTF-8 decoding rejects malformed sequences and invalid Unicode scalar encodings.
- [x] LF and CRLF line endings are normalized into logical Document lines.
- [x] Empty files are represented as zero-line Documents.
- [x] File loading stages changes and preserves the destination Document on failure.
- [x] Insertion into a zero-line Document creates its first logical line.
- [x] Splitting a zero-line Document at `(0, 0)` creates two empty lines.
- [x] Renderer safely draws a zero-line Document as a blank document area.
- [x] Editor State loads a file and resets Cursor, Viewport, and Dirty Flag only after success.
- [x] The application accepts an optional filename and loads it before the initial frame.
- [x] Regression tests cover zero-line editing/rendering and Editor State load success/failure.
- [x] CMake build and `smoke`, `editing`, and `file` CTest targets pass.

## Current Runtime Capabilities

The application can now:

- initialize the terminal and enter raw mode;
- preserve and restore the original terminal configuration;
- obtain the terminal dimensions;
- initialize the Editor State and its Document;
- render the visible document through a Screen Buffer;
- insert printable ASCII characters into the Document;
- advance the Cursor after successful insertion;
- move the Cursor left and right within the current logical line;
- move the Cursor up and down while preserving its preferred column;
- maintain vertical and horizontal Viewport offsets to keep the Cursor visible;
- render the visible document region using the Viewport offsets;
- reserve the terminal's final physical column to avoid the observed right-edge crash in WSL;
- delete the character before the Cursor using Backspace, or merge with the previous line when Backspace is pressed at column zero and a previous line exists;
- split logical lines using Enter or line feed;
- mark the Document dirty after successful insertion, deletion, line splitting, or line merging;
- redraw the screen after editing and navigation;
- exit using `Ctrl-Q`;
- clean up initialized resources and return control to the shell.

## Verification

### Build and automated tests

- [x] CMake configuration and build succeeded for the current Chapter 49 implementation
- [x] `smoke` automated test passed
- [x] `editing` automated test passed
- [x] Regression tests cover repeated vertical and horizontal movement, Viewport visibility, and Renderer positioning
- [x] User confirmed the build succeeds and registered tests pass
- [x] User manually verified that the vertical scrolling issue is resolved
- [x] User manually verified that typing past the right edge no longer closes the editor in WSL
- [ ] Comprehensive automated tests across all Document, Cursor, Screen Buffer, Renderer, and terminal behaviors
- [ ] Integration and regression testing across broader failure paths
- [ ] Terminal resizing and frame reinitialization
- [ ] Broader end-to-end verification during the hardening milestone

The current build and registered tests pass, and the user has confirmed the two scrolling fixes in the actual WSL terminal. Comprehensive correctness, allocation-failure coverage, terminal resizing, and later end-to-end hardening remain outstanding.

## Implementation Status Summary

| Area                                                        | Status                                                                                              |
| ----------------------------------------------------------- | --------------------------------------------------------------------------------------------------- |
| Repository and Git workflow                                 | Established                                                                                         |
| CMake and C99 configuration                                 | Established                                                                                         |
| Terminal initialization, raw mode, restoration, and cleanup | Implemented                                                                                         |
| Application lifecycle and event loop                        | Implemented; input handling remains temporary                                                       |
| Editor State                                                | Implemented                                                                                         |
| Document                                                    | Vector of logical lines; insertion, deletion, splitting, and merging implemented                    |
| Cursor                                                      | Validated logical position, horizontal and vertical movement, preferred-column behavior implemented |
| Viewport                                                    | Vertical and horizontal offsets and Cursor visibility maintenance implemented                       |
| Screen Buffer                                               | Implemented                                                                                         |
| Renderer                                                    | Full redraw of the visible document region; reserves the final physical terminal column             |
| Printable ASCII insertion                                   | Implemented                                                                                         |
| Backspace character deletion and line merging               | Implemented                                                                                         |
| Enter and logical-line splitting                            | Implemented                                                                                         |
| Dirty Flag                                                  | Updated after successful insertion, deletion, splitting, and merging                                |
| Full document navigation                                    | Implemented for the current ASCII-only model                                                        |
| Viewport and scrolling                                      | Implemented; vertical and horizontal behavior manually verified in WSL                              |
| File loading                                                | Implemented: UTF-8 decode, LF/CRLF normalization, empty files, startup filename                      |
| File saving                                                 | File Manager + Editor State API + Ctrl-S for startup filename implemented; Save As and feedback pending |
| Dedicated Input Parser and command pipeline                 | Not implemented; basic input recognition remains in the event loop                                  |
| Status Bar and Message Bar                                  | Not implemented                                                                                     |
| Terminal resize handling                                    | Not implemented                                                                                     |
| Unicode-aware input and display-width mapping               | Not implemented                                                                                     |
| Automated tests                                             | `smoke`, `editing`, and `file` pass; broader coverage pending                                        |
| Hardening and final integration                             | Pending                                                                                             |

## Current Milestone

**Chapter 51 — Saving Files: implemented through Section 51.5 for the existing-file workflow**

The File Manager serializes the Document as UTF-8 with canonical LF line endings, writes through a same-directory temporary file, checks writes and close, and atomically replaces the destination with `rename()`. Editor State clears the Dirty Flag only after successful replacement. Ctrl-S saves to the startup filename. Failure tests cover invalid scalar data and replacement failure, including destination/state preservation and temporary-file cleanup. Save As and Message Bar feedback remain pending.

## Next Milestone

**Chapter 52 — Completing the Editor Interface**

Chapter 51 is implemented through Section 51.5 for saving an existing file. Next, implement the Status Bar and Message Bar according to the project architecture. Use the Message Bar to surface save success and failure without adding direct status output to `main.c`. Save As for unnamed Documents remains out of scope unless the project requirements explicitly call for it.

## Current Limitations

- Only printable ASCII characters are accepted for insertion.
- The terminal Delete key is not implemented; Backspace is supported.
- UTF-8-aware input and Unicode display-width mapping are not implemented.
- Tab expansion is not implemented.
- Ctrl-S saves to the filename supplied at startup; Save As for an unnamed Document is not implemented.
- Save feedback is pending the Message Bar.
- Input handling remains temporarily inside the application event loop rather than a dedicated Input Parser, Input Engine, and Command Engine pipeline.
- The Status Bar and Message Bar are not implemented.
- Terminal resize handling and frame reinitialization are not implemented.
- Comprehensive automated testing, broader failure-path regression testing, hardening, and final integration remain future work.
- The Renderer deliberately reserves the terminal's final physical column to avoid the right-edge wrapping behavior observed in WSL; this reduces the usable rendering width by one column.

## Chapter 51 — Saving Files (Sections 51.2–51.5)

- [x] File Manager UTF-8 serialization with invalid-code-point validation.
- [x] Canonical LF output after each logical line; zero-line Document produces a zero-byte file.
- [x] Temporary file in the destination directory, complete buffered writes, close checking, and atomic rename replacement.
- [x] `editor_state_save_file()` delegates to the File Manager and clears `is_dirty` only after success.
- [x] Failed save preserves the in-memory Document, Dirty Flag, and prior destination contents in the tested failure case.
- [x] CMake build and all four CTest targets (`smoke`, `editing`, `file`, `file_save`) pass.
- [x] Connect Ctrl-S to the Editor State save API for a startup filename.
- [x] Verify end-to-end save behavior through a pseudo-terminal session.
- [ ] Add visible success/failure feedback when the Message Bar is implemented.
- [ ] Decide whether Save As belongs in the implementation scope; currently Ctrl-S is a no-op for an unnamed Document.
- [x] Complete the current failure-path review; broader injected write/close failures remain part of later hardening.


### Section 51.4 — Event Loop Integration

- [x] Added the Ctrl-S control byte (`0x13`) to the current direct input loop.
- [x] Passed the optional startup filename to the Event Loop and save to that path.
- [x] Save failure does not terminate the Event Loop or clear the Dirty Flag.
- [x] CMake build and all four CTest targets pass.
- [x] Pseudo-terminal check verified edited contents are persisted after Ctrl-S.
- [ ] Visible save feedback remains pending the Message Bar.
- [ ] Save As for an unnamed Document is not implemented.

### Section 51.5 — Failure-Path Review

- [x] Verify invalid scalar data fails before destination replacement.
- [x] Verify a failed save preserves the Dirty Flag and in-memory Document.
- [x] Verify a failed save preserves existing destination contents.
- [x] Verify a rename/replacement failure leaves an existing directory intact.
- [x] Verify failed replacement cleans up its temporary file.
- [x] Reconfigure and rebuild the project successfully.
- [x] Pass all four CTest targets.
- [x] Pass a separate warning-clean build with `-Wall -Wextra -Wpedantic -Werror`.
- [x] Verify successful save and recoverable replacement failure in pseudo-terminal integration checks.
- [x] Keep visible save feedback deferred to the Message Bar implementation.
