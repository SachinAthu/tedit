# Implementation Status

## Project Status

**Active — Volume X implementation**

Chapters 45–51 and Chapter 52 sections 52.2–52.8 are implemented in the current source snapshot. The interface includes a bottom Status Bar, a white Message Bar with black text, filename editing with cursor navigation and Delete, and dirty-quit confirmation. Ctrl-S saves named documents to their current path and opens filename entry for unnamed documents; no separate Save As command is implemented. Debug and warning-clean builds pass all eight CTest targets, and pseudo-terminal checks cover the latest interface flows.

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
| Application lifecycle and event loop                        | Implemented; complete parser-to-command pipeline                                                       |
| Editor State                                                | Implemented                                                                                         |
| Document                                                    | Vector of logical lines; insertion, deletion, splitting, and merging implemented                    |
| Cursor                                                      | Validated logical position, horizontal and vertical movement, preferred-column behavior implemented |
| Viewport                                                    | Vertical and horizontal offsets and Cursor visibility maintenance implemented                       |
| Screen Buffer                                               | Implemented                                                                                         |
| Renderer                                                    | Full redraw; bottom Status Bar, styled Message Bar, and separator row                                 |
| Unicode character insertion                                | UTF-8 input events and Unicode scalar insertion implemented                                              |
| Backspace character deletion and line merging               | Implemented                                                                                         |
| Enter and logical-line splitting                            | Implemented                                                                                         |
| Dirty Flag                                                  | Updated after successful insertion, deletion, splitting, and merging                                |
| Cursor navigation                                           | Arrow-key movement implemented; Home, End, Page Up, and Page Down deferred                              |
| Viewport and scrolling                                      | Implemented; vertical and horizontal behavior manually verified in WSL                              |
| File loading                                                | Implemented: UTF-8 decode, LF/CRLF normalization, empty files, startup filename                      |
| File saving                                                 | Safe save implemented; Ctrl-S saves current path or opens filename entry for unnamed documents       |
| Dedicated Input Parser and command pipeline                 | Input Parser, Input Engine, and Command Engine implemented                                             |
| Status Bar and Message Bar                                  | Implemented; Status Bar bottom, white Message Bar, one-row separator                                   |
| Terminal resize handling                                    | Not implemented                                                                                     |
| Unicode-aware input and display-width mapping               | Not implemented                                                                                     |
| Automated tests                                             | Eight CTest targets pass in Debug and warning-clean builds                                             |
| Hardening and final integration                             | Pending                                                                                             |

## Current Milestone

**Chapter 52.8 — Interface Refinement and Unsaved-Changes Confirmation**

The current implementation has the complete input pipeline, filename entry for unnamed documents, refined interface layout, and dirty-quit confirmation. Ctrl-S saves a named document to its current path; no separate Save As command is implemented. Eight CTest targets pass in Debug and warning-clean builds, and pseudo-terminal integration covers the changed workflows.

## Next Milestone

**Chapter 53 — Hardening.** Focus on invariants, boundary cases, failure paths, sanitizers, and broader regression tests. Home, End, Page Up, Page Down, document Delete, and Tab remain follow-up work for the earlier navigation/editing milestones.

## Current Limitations

- Non-ASCII filename bytes are preserved for file operations but rendered as `?` in the Status Bar and filename-entry prompt.
- Document Delete, Home, End, Page Up, Page Down, and Tab are not implemented yet; Delete-key input is currently handled only during filename entry.
- Ctrl-S on a named document saves to the current path. A separate Save As command is intentionally deferred.
- Terminal resizing and frame reinitialization are not implemented.
- Broader allocation-failure, sanitizer, and terminal-size integration coverage remains part of hardening.

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

## Chapter 52 — Section 52.5: Input Engine and Command Dispatch (2026-10-15)

- [x] Added `src/input/input_engine.h/.c` to map abstract input events to editor commands.
- [x] Added `src/command/command_engine.h/.c` to execute editing, movement, save, and quit commands.
- [x] Refactored `main.c` to orchestrate Input Handler → Input Parser → Input Engine → Command Engine → Renderer.
- [x] Added `test_input_engine` and `test_command_engine` CTest targets.
- [x] Debug build and all seven tests pass.
- [x] Warning-clean build with `-Wall -Wextra -Wpedantic -Werror` and all seven tests pass.
- [x] Pseudo-terminal integration verified insertion, Ctrl-S save, and Ctrl-Q exit.
- [ ] Filename-entry and Save As remain unimplemented.

Next: Chapter 52.6 — integration review and key-binding boundary before filename-entry and Save As.


### Chapter 52.7 — Filename Entry and Save As

- [x] Added `EDITOR_COMMAND_CANCEL` and `INPUT_EVENT_ESCAPE`; standalone Escape is emitted when the Input Handler timeout expires while the parser is in the Escape state.
- [x] Ctrl-S starts filename entry when Editor State has no associated path. Ctrl-S on an associated document continues saving to its current path.
- [x] Added bounded filename entry in Editor State; printable Unicode input is encoded as UTF-8, Backspace removes one UTF-8 code point, Enter confirms, and Escape cancels.
- [x] Renderer displays the `Save as:` prompt in the Status Bar and places the terminal cursor at the entry position.
- [x] Added `editor_state_save_as()`: allocates the new path before saving, then updates file association and clears the Dirty Flag only after successful atomic save.
- [x] Save failure keeps filename entry active for correction/retry and preserves file association, Document, and Dirty Flag.
- [x] Cancellation closes filename entry without changing the file association, Document, or Dirty Flag.
- [x] Added tests for Save As success, Backspace, failed save, cancellation, association preservation, and Escape parsing/dispatch.
- [x] Debug build and all eight CTest targets pass.
- [x] Warning-clean build (`-Wall -Wextra -Wpedantic -Werror`) and all eight tests pass.
- [x] Pseudo-terminal integration checks verify Save As persistence and Escape cancellation.

Current next milestone: Chapter 53 — Hardening. The user explicitly deferred a separate Save As command. Home, End, Page Up, Page Down, document Delete, and Tab remain follow-up work for the earlier navigation/editing milestones.


### Chapter 52.8 — Interface Refinement and Unsaved-Changes Confirmation

- [x] Moved the Status Bar to the bottom terminal row.
- [x] Added one blank separator row between document text and the Message Bar.
- [x] Rendered the Message Bar with black foreground and white background; filename-entry cursor uses the same rendition.
- [x] Moved filename entry to the Message Bar and tracks a byte cursor at UTF-8 code-point boundaries.
- [x] Added Left/Right navigation, insertion at the cursor, Backspace, and Delete (`CSI 3 ~`) to filename entry.
- [x] Added dirty-quit confirmation: `Y` saves and quits, `N` discards and quits, Escape cancels.
- [x] Choosing save for an unnamed Document opens filename entry; quit occurs only after successful saving.
- [x] Added tests for filename cursor editing, UTF-8 boundary deletion, named/unnamed save-before-quit, cancellation, and discard.
- [x] Debug build and all eight CTest targets pass.
- [x] Warning-clean build (`-Wall -Wextra -Wpedantic -Werror`) and all eight tests pass.
- [x] Pseudo-terminal checks pass for filename editing/saving, Message Bar styling, and dirty-quit workflows.

Next: Chapter 53 — Hardening. Document navigation keys and Tab remain deferred to their earlier implementation milestones. No separate Save As command is planned for the current baseline.
