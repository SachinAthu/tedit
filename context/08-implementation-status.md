# Implementation Status

## Project Status

**Chapter 55 implementation complete as a reviewable release candidate; public distribution awaits a license decision**

Chapters 45–51, Chapter 52 sections 52.2–52.8, Chapter 53 sections 53.2–53.11, and Chapter 54 sections 54.1–54.7 are complete for the current implementation scope. The diagnostic logging follow-up adds `src/common/logger.{h,c}`, the `ERROR`, `WARN`, `INFO`, and `DEBUG` levels, timestamped component-labelled records, and best-effort logging to `log.txt`. The logger opens the file in write mode. Development builds enable application logging by default; Release builds omit application log calls, and `TEDIT_ENABLE_LOGGING=OFF` disables them explicitly. The editor and logger are implemented in C99. The optional Python POSIX pseudo-terminal test is only an external integration harness.

The current CMake configuration registers ten C test targets and a POSIX pseudo-terminal integration test when Python 3 is available. The implementation record reports all eleven CTest tests passing in Debug with diagnostics/logging, sanitizer-enabled, warning-clean, Release, and explicit logging-disabled configurations. Remaining hardening limitations include deterministic allocation-failure injection, exhaustive terminal-write failure injection, and known Unicode/filename display limitations. **Chapter 55 — Final Integration and Release is complete for the current Linux/POSIX verification scope.**

> **Status note:** Chapter-specific entries later in this document preserve historical milestone records, including the test counts and “next step” notes that were accurate at the time. The current status, capability, verification, summary, milestone, and limitation sections above/below take precedence for the present implementation snapshot.

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

The current implementation can:

- initialize the POSIX terminal, enter raw mode, enter the alternate screen, and restore terminal state during cleanup;
- query terminal dimensions and handle `SIGWINCH` by creating a replacement Renderer before replacing the active Renderer;
- handle `SIGINT`, `SIGTERM`, and `SIGHUP` through signal-safe flags and normal event-loop cleanup;
- load an optional startup file, decode valid UTF-8, normalize LF/CRLF input, and preserve the destination Document on load failure;
- save UTF-8 text through a temporary file and atomic replacement, preserving the Dirty Flag and destination on tested failure paths;
- maintain a logical-line Document and a Cursor measured in Unicode code points;
- parse UTF-8 keyboard input and common terminal sequences through the Input Parser → Input Engine → Command Engine pipeline;
- insert text, insert tabs, split and merge lines, delete code points, and navigate with arrows, Home, End, Page Up, and Page Down;
- preserve the preferred visual column during vertical navigation and map logical positions to visual columns using tab stops, wide-code-point widths, and combining marks;
- maintain vertical and horizontal Viewport offsets to keep the Cursor visible;
- render through a contiguous Screen Buffer using full-frame redraws, a bottom Status Bar, a styled Message Bar, and a separator row;
- prompt before quitting a dirty Document and support filename entry when saving an unnamed Document;
- write timestamped diagnostic records to `log.txt` in development builds, with severity filtering and best-effort failure handling;
- run invariant checks, regression tests, sanitizer builds, warning-clean builds, Release tests, and optional POSIX pseudo-terminal integration tests.

The logger does not own or mutate Editor State. Input text is not written to the log; character events record the Unicode code point rather than the document content.

## Verification

### Current build and automated-test status

- [x] The implementation record reports a successful Debug build with diagnostics and logging enabled.
- [x] The implementation record reports a successful AddressSanitizer/UndefinedBehaviorSanitizer build with leak detection and halt-on-error enabled.
- [x] The implementation record reports a successful warning-clean build using `-Wall -Wextra -Wpedantic -Werror`.
- [x] The implementation record reports a successful Release build.
- [x] The implementation record reports a successful Debug build with logging explicitly disabled.
- [x] All ten C test targets pass in the reported configurations.
- [x] The POSIX pseudo-terminal integration test passes when Python 3 is available, giving eleven CTest tests in that environment.
- [x] Logger tests cover severity filtering, timestamps, unavailable log paths, and best-effort write failure handling.
- [x] Pseudo-terminal integration covers Ctrl-Q, resize handling, signal termination, terminal restoration, startup-load failure, and logging expectations.
- [x] Release and explicitly logging-disabled integration runs do not create `log.txt`; development integration verifies lifecycle and failure records.

### Remaining verification work

- [ ] Add deterministic allocator-failure injection across allocation sites.
- [ ] Add exhaustive terminal-output write-failure injection and recovery coverage.
- [ ] Expand terminal-size integration coverage beyond the currently tested resize scenarios.
- [ ] Continue Unicode validation for grapheme clusters, emoji sequences, and terminal-specific width behavior.

These verification statements summarize the current implementation records. Historical chapter sections below retain the test counts that applied when those milestones were completed.

## Implementation Status Summary

| Area                                                                          | Current status                                                                                                       |
| ----------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------- |
| Repository, CMake, and C99                                                    | Established                                                                                                          |
| Terminal initialization, raw mode, restoration, alternate screen, and cleanup | Implemented; lifecycle and signal cleanup covered by pseudo-terminal tests                                           |
| Resize handling                                                               | Implemented with signal-safe notification and transactional Renderer replacement                                     |
| Editor State and Dirty Flag                                                   | Implemented; tested save/edit transitions preserve dirty state on covered failures                                   |
| Document                                                                      | Logical-line vector; insertion, deletion, split, merge, and UTF-8 code-point editing implemented                     |
| Cursor and navigation                                                         | Logical positions, horizontal/vertical movement, Home/End, Page Up/Down, preferred visual column                     |
| Viewport                                                                      | Vertical and horizontal offsets; Cursor visibility maintained                                                        |
| Screen Buffer and Renderer                                                    | Implemented; full redraw, tab expansion, wide-character continuation cells, combining marks, Status Bar, Message Bar |
| File Manager                                                                  | UTF-8 loading/saving, LF/CRLF normalization, temporary-file save and replacement                                     |
| Input Parser / Input Engine / Command Engine                                  | Integrated command pipeline implemented                                                                              |
| Filename entry and unsaved-change confirmation                                | Implemented for unnamed-document save and dirty-quit workflow                                                        |
| Diagnostic assertions                                                         | Implemented for internal invariants                                                                                  |
| Diagnostic logging                                                            | Implemented in `src/common/logger.{h,c}`; write-mode `log.txt`, four levels, timestamps, component labels            |
| Automated tests                                                               | Ten C test targets plus optional POSIX pseudo-terminal test; eleven CTest tests when Python 3 is available           |
| Remaining hardening                                                           | Allocation-failure injection, exhaustive terminal-write failure injection, broader resize matrix                     |
| Project status                                                                 | Chapter 55 implementation complete as a release candidate; license decision remains open                           |

## Current Milestone

**Chapter 55 — Final Integration and Release complete for the current verification scope.**

The current implementation includes Document/Cursor/Viewport invariant tests, pre-serialization storage validation, internal assertions, malformed-input parser recovery, terminal restoration and resize handling, POSIX pseudo-terminal lifecycle tests, and the persistent diagnostic logger. The logger uses write mode and does not modify Editor State. The reported build matrix passes all eleven CTest tests when Python 3 is available.

## Release Readiness Gate

**Before public distribution:** select and add an appropriate project license, then rerun the release checks against the final licensed source snapshot.

Chapter 55 reviewed architecture/dependency direction, removed the Editor State → File Manager dependency, validated GCC/Clang and Debug/Release/sanitizer/logging configurations, tested a clean extracted archive, updated documentation, and prepared a source release-candidate archive. Cross-platform OS testing was not performed. Public redistribution remains gated on selecting a license.

### Chapter 55.1 — Architecture and Dependency Review

- [x] Reviewed the current module responsibilities and the documented dependency rules.
- [x] Identified `src/state/editor_state.c` depending on `src/file/file.h`, contrary to the Core Layer independence rule.
- [x] Confirmed that the existing architecture already specifies the intended direction; no new ADR was necessary.

### Chapter 55.2 — Module and Interface Cleanup

- [x] Removed File Manager and logger includes and file I/O calls from `EditorState` implementation.
- [x] Added Application Layer coordination APIs for load, save, and Save As to the Command Engine.
- [x] Routed startup loading and interactive save paths through the Command Engine.
- [x] Preserved staged-load, Dirty Flag, and Save As file-association failure guarantees.
- [x] Updated state-level file-operation tests to exercise the Application Layer coordinator.
- [x] GCC Debug, sanitizer-enabled, warning-clean, Release, and logging-disabled builds pass all eleven CTest tests in this POSIX/Python environment.
- [x] Clang warning-clean build and a separate clean Debug build pass all eleven CTest tests.
- [x] Updated README and added Chapter 55 release-candidate notes; license selection remains open.

### Chapter 55.3 — Portability Checks

- [x] GCC 14.2.0 and Clang strict-warning builds compile on the current Linux/POSIX host.
- [x] Normalized missing final newlines in eleven existing C source/header files after Clang exposed `-Wnewline-eof` under `-Werror`.
- [x] Confirmed all eleven tests pass under the Clang warning-clean build.
- [x] Recorded that testing on other operating systems/POSIX implementations remains outside this environment's verification scope.

### Chapter 55.4 — Debug and Release Build Configurations

- [x] Debug with logging enabled passes all eleven tests.
- [x] Release passes all eleven tests.
- [x] Debug with logging disabled passes all eleven tests.
- [x] AddressSanitizer/UndefinedBehaviorSanitizer build passes all eleven tests with leak detection and halt-on-error settings.
- [x] GCC and Clang warning-clean builds pass all eleven tests.

### Chapter 55.5 — Clean Build and Reproducibility Checks

- [x] Configured and built in a fresh separate Debug build directory.
- [x] Extracted the packaged source archive to a separate directory, configured from scratch, built, and passed all eleven tests.

### Chapter 55.6 — Packaging and Release Artifacts

- [x] Created a source-only release-candidate ZIP without build outputs or `log.txt`.
- [x] Generated a SHA-256 checksum file.
- [x] Recorded that the archive does not grant reuse or redistribution rights because no license has been selected.

### Chapter 55.7 — User and Developer Documentation

- [x] Rewrote README build/run instructions to match current behavior and supported options.
- [x] Documented key bindings, architecture boundaries, known limitations, and optional Python integration-test behavior.
- [x] Added `RELEASE_NOTES.md` for scope, verification, and limitations.

### Chapter 55.8 — Final End-to-End Validation

- [x] The pseudo-terminal integration test passes in all tested build configurations.
- [x] The extracted release-candidate archive passes the full eleven-test suite.

### Chapter 55.9 — Final Implementation Status

- [x] Updated current state, implementation status, implementation notes, and known issues.
- [x] Recorded the dependency correction and the evidence supporting the release-candidate status.
- [x] Kept unresolved hardening limitations and the missing license decision explicit.

## Current Limitations

- Non-ASCII filename bytes are preserved for file operations but rendered as `?` in the Status Bar and filename-entry prompt.
- Unicode display width uses POSIX `wcwidth()` where available and fallback ranges for common combining and wide-character blocks. Grapheme clusters, emoji ZWJ sequences, a complete versioned Unicode width database, and terminal-specific width differences remain limitations.
- Ctrl-S saves an associated Document to its current path. There is no separate Save As command for an already-associated Document.
- Deterministic allocation-failure injection and exhaustive terminal-output write-failure injection are not implemented.
- No license has been selected; the source archive is a reviewable release candidate and does not grant redistribution or reuse permissions.
- The logger uses the fixed relative path `log.txt`; custom log-path configuration and explicit runtime log-level configuration are not implemented.
- Logging is best-effort and buffered. If the filesystem reports a write failure, the editor continues, and the final buffered records may be lost.
- The pseudo-terminal integration test requires a POSIX environment and Python 3; the C implementation itself does not depend on Python.

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

Current next milestone: Chapter 53.

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

Next: Chapter 53.

## Chapter 53.2 — Terminal Sequences for Additional Keys

- [x] Added abstract Input Events for Home, End, Page Up, Page Down, and Tab.
- [x] Parsed common terminal sequences for Home (`CSI H`, `CSI 1 ~`, `CSI 7 ~`) and End (`CSI F`, `CSI 4 ~`, `CSI 8 ~`).
- [x] Parsed Page Up (`CSI 5 ~`) and Page Down (`CSI 6 ~`).
- [x] Recognized Tab (`0x09`) as `INPUT_EVENT_TAB`.
- [x] Added Input Parser regression tests for each supported sequence.
- [x] Debug build and all eight CTest targets pass.
- [x] Map Home and End events through Input Engine and Command Engine.
- [x] Add Cursor Home/End operations using logical code-point columns.
- [x] Add Cursor, Command Engine, Input Engine, and end-to-end input regression tests.
- [x] Debug build and all eight CTest targets pass.

Next: Chapter 53.4 — Delete-Key Editing.

## Chapter 53.5 — Page Up and Page Down Navigation

- [x] Added Cursor operations for page-sized vertical movement.
- [x] Page distance comes from `Renderer.document_rows` and is supplied by the Event Loop.
- [x] Mapped Page Up and Page Down input events to explicit Editor Commands.
- [x] Preserves the preferred visual column across page movement, including short destination lines; ADR-034 supersedes the original logical-column policy for vertical navigation.
- [x] Added command and input-pipeline regression tests for boundaries, no-op behavior, Dirty Flag preservation, and terminal sequences.
- [x] Debug warning-clean build passes with `-Wall -Wextra -Wpedantic -Werror`.
- [x] All eight CTest targets pass.

## Chapter 53.6 — Tab Insertion and Configured Tab Width

- [x] Added `EditorConfiguration` to `EditorState` with `DEFAULT_TAB_WIDTH` set to 4.
- [x] Mapped `INPUT_EVENT_TAB` to the dedicated `EDITOR_COMMAND_INSERT_TAB` command.
- [x] Inserted U+0009 as one logical Document code point; the command sets Dirty Flag and redraw only after a successful insertion.
- [x] Kept Tab inert in filename-entry mode by using a distinct command type.
- [x] Expanded tabs into screen-buffer spaces through the next configured tab stop.
- [x] Added a Renderer helper to convert logical cursor position to visual display column.
- [x] Passed visual cursor columns to Viewport visibility calculations and terminal cursor positioning.
- [x] Updated Viewport coordinate documentation to define `viewport_column` as a visual column.
- [x] Added tests for Tab mapping/insertion, configured tab stops, screen-buffer expansion, cursor placement, and the input pipeline.
- [x] Debug build and all eight CTest targets pass.
- [x] Warning-clean build (`-Wall -Wextra -Wpedantic -Werror`) and all eight CTest targets pass.
- [x] Pseudo-terminal check confirms Tab insertion, Ctrl-S saving, and Ctrl-Q exit; the saved file preserves U+0009.

Note: this milestone provides the built-in configuration value and consuming API field. Reading user configuration files and command-line configuration overrides remain outside this milestone.

Next: Chapter 53.7 — UTF-8 Keyboard Input and Character-Boundary Editing.

## Chapter 53.7 — UTF-8 Keyboard Input and Character-Boundary Editing

- [x] Verified UTF-8 parser behavior for two-, three-, and four-byte Unicode scalar values.
- [x] Added malformed-sequence coverage for invalid lead bytes, overlong encodings, surrogate values, values above U+10FFFF, and interrupted sequences.
- [x] Verified parser timeout clears incomplete UTF-8 state and subsequent ASCII input remains usable.
- [x] Added Command Engine regression coverage proving Backspace removes one stored Unicode code point at a time.
- [x] Added end-to-end parser-to-command tests proving a four-byte UTF-8 sequence produces one Document code point and Backspace removes that code point as one character.
- [x] Debug warning-clean build (`-Wall -Wextra -Wpedantic -Werror`) passes.
- [x] All eight CTest targets pass.

Next: Chapter 53.8 — Logical Cursor Positions and Visual Display Columns.

**Chapter 53.8 implementation result:** The Screen Buffer now represents wide-character continuation cells and stores combining marks alongside their base cell. The Renderer calculates logical-to-visual positions using tab stops and Unicode display widths, and emits wide and combining code points without counting UTF-8 bytes as terminal columns. Tests cover CJK width, combining marks, and cursor placement.

## Chapter 53.8 — Logical Cursor Positions and Visual Display Columns

- [x] Kept Cursor and Document columns measured in logical Unicode code points.
- [x] Added display-width calculation for ordinary, wide, and combining code points; retained configured tab-stop expansion.
- [x] Extended ScreenCell with wide-character continuation and attached combining-mark storage.
- [x] Updated frame output to emit wide characters once, skip continuation cells, and emit combining marks with their base cell.
- [x] Updated terminal cursor mapping and retained visual-column Viewport integration.
- [x] Added regression coverage for CJK width, combining-mark attachment, UTF-8 output, and cursor display columns.
- [x] Debug build and all eight CTest targets pass.
- [x] Warning-clean build (`-Wall -Wextra -Wpedantic -Werror`) and all eight CTest targets pass.

Next: Chapter 53.9 — Input Engine and Command Engine Integration.

## Chapter 53.9 — Input Engine and Command Engine Integration

- [x] Audited the complete parser → Input Engine → Command Engine path for the Chapter 53 keyboard additions.
- [x] Added end-to-end coverage for alternate Home (`CSI 1 ~`) and End (`CSI 8 ~`) sequences reaching Cursor navigation.
- [x] Added end-to-end coverage proving Tab, Home, End, Page Up, and Page Down do not mutate the Document or filename when the filename-entry mode is active.
- [x] Verified Escape timeout still routes to the Cancel command and closes filename entry.
- [x] Input Engine tests explicitly verify that `page_rows` is zero after translation; the Event Loop supplies `Renderer.document_rows` before command execution.
- [x] Debug build and all eight CTest targets pass.
- [x] Warning-clean build (`-Wall -Wextra -Wpedantic -Werror`) and all eight CTest targets pass.
- [x] Pseudo-terminal check exercised printable input, alternate Home/End, Page Up/Down, Tab, and dirty-quit discard; process exited cleanly and restored the alternate screen.

Next: Chapter 53.10 — Boundary, Regression, and Pseudo-Terminal Tests.

## Chapter 53.10 — Boundary, Regression, and Pseudo-Terminal Tests

- [x] Added input-pipeline regression for Delete at the final empty line (no-op, no Dirty Flag change, no redraw).
- [x] Added input-pipeline regression for Delete at the end of a non-final line (merges the next line, marks dirty, and redraws).
- [x] Retained existing coverage for Home/End boundaries, Page Up/Down document boundaries, preferred-column preservation, Unicode code-point editing, tab visual columns, viewport validation, and filename-entry isolation.
- [x] Debug CMake build and all eight CTest targets pass.
- [x] Warning-clean build (`-Wall -Wextra -Wpedantic -Werror`) and all eight CTest targets pass.
- [x] Pseudo-terminal editing/save/exit test passed; saved output was exactly `bc\t\n`, and alternate-screen restoration was observed.

Chapter 53.11 — Resulting Implementation State is complete. Next: Chapter 54 — Hardening the Editor.

## Chapter 53.11 — Resulting Implementation State

- [x] Recorded the final Chapter 53 feature set and architectural boundaries.
- [x] Confirmed Home/End, Delete, Page Up/Page Down, and Tab are integrated through the established input and command pipeline.
- [x] Confirmed UTF-8 input is decoded into Unicode scalar values and editing uses logical code-point positions.
- [x] Confirmed Renderer visual-column mapping covers configured tab stops, wide code points, and combining marks, while recording Unicode width limitations.
- [x] Updated current-state, implementation-status, implementation-notes, and known-issues records.
- [x] Fresh Debug build and all eight CTest targets passed.
- [x] Warning-clean build (`-Wall -Wextra -Wpedantic -Werror`) and all eight CTest targets passed.
- [x] Pseudo-terminal workflow verified exact saved bytes `bc\t\n`, exit status 0, and alternate-screen restoration.

Next: Chapter 54 — Hardening the Editor.

### Follow-up — Status Bar and Vertical Navigation

- [x] Status Bar reports line, logical column, and visual display column explicitly.
- [x] Added shared `src/text/text_layout.{h,c}` calculations for tab stops and Unicode code-point widths.
- [x] Vertical cursor and page movement preserve a preferred visual column across tabbed, wide-character, and short lines.
- [x] Added regression coverage for a status position after a tab and vertical movement across tabbed lines with a short-line round trip.
- [ ] Continue with Chapter 54 hardening work after this focused refinement.

## Chapter 54 — Hardening the Editor

### Chapter 54.1 — Establishing the Hardening Build

- [x] Added the `TEDIT_ENABLE_SANITIZERS` CMake option for GCC/Clang.
- [x] Debug build and all eight CTest targets pass.
- [x] AddressSanitizer/UndefinedBehaviorSanitizer build and all eight CTest targets pass with leak detection enabled.

### Chapter 54.2 — Document, Cursor, and Viewport Invariants

- [x] Added test-side Document structure checks for line-array and line-storage consistency.
- [x] Verified rejected Document edits leave contents and structural invariants unchanged.
- [x] Verified rejected Cursor position assignments preserve all Cursor position/preference fields.
- [x] Verified repeated forward/reverse Viewport adjustments preserve cursor visibility.
- [x] Debug, sanitizer-enabled, and warning-clean builds pass all eight CTest targets.

Next: Chapter 54.3 — Allocation and File-Operation Failures.

## Chapter 54.3 — Allocation and File-Operation Failures

- [x] Validate Document line-array and line-buffer metadata before file serialization.
- [x] Reject malformed storage without modifying the existing destination file.
- [x] Verify failed saves preserve the Dirty Flag and in-memory Document.
- [x] Verify a file-read error preserves the destination Document.
- [x] Debug build and all eight CTest targets pass.
- [x] AddressSanitizer/UndefinedBehaviorSanitizer build and all eight CTest targets pass with leak detection enabled.
- [x] Warning-clean build (`-Wall -Wextra -Wpedantic -Werror`) and all eight CTest targets pass.

Next: Chapter 54.4 — Assertions and Debug Diagnostics. Deterministic allocation-failure injection remains open.

## Chapter 54.4 — Assertions and Debug Diagnostics

- [x] Added `TEDIT_ASSERT` for internal invariant checks while retaining explicit validation for public inputs.
- [x] Added `TEDIT_ASSERT` for internal invariant checks; the later diagnostic logger is recorded in the Diagnostic Logging follow-up below.
- [x] Added post-mutation Document storage assertions for successful initialization, insertion, deletion, split, and merge operations.
- [x] Added diagnostic messages for rejected save paths, inconsistent Document storage, and temporary-file creation failures.
- [x] Corrected test targets to keep `assert()` active in Release builds on GCC/Clang, without changing production-target `NDEBUG` behavior.
- [x] Debug, sanitizer-enabled, warning-clean, and Release builds pass all eight CTest targets.
- [x] Continue with Chapter 54.5 — Malformed Input and Parser Recovery.

## Chapter 54.5 — Malformed Input and Parser Recovery

- [x] Reprocess standalone control bytes that interrupt malformed/incomplete CSI sequences rather than swallowing them.
- [x] Verify Ctrl-S recovery from a numeric CSI parameter and Escape restart after an incomplete CSI prefix.
- [x] Verify oversized CSI parameter sequences are discarded safely and subsequent input is recognized.
- [x] Verify null parser arguments are rejected without dereferencing them.
- [x] Debug with diagnostics, sanitizer-enabled, warning-clean, and Release builds pass all eight CTest targets.

Next: Chapter 54.6 — Terminal Restoration and Resize Handling.

## Chapter 54.6 — Terminal Restoration and Resize Handling

- [x] Added `terminal_get_size()` with null-argument validation and no partial output updates on failure.
- [x] Corrected terminal restoration to use `TCSAFLUSH`, retry `EINTR`, and retain the raw-mode flag if restoration fails.
- [x] Made alternate-screen cleanup null-safe.
- [x] Added signal-safe flags for resize and termination signals; handlers perform no terminal I/O or allocation.
- [x] Added transactional Renderer replacement on terminal resize and revalidated Cursor visibility through the existing redraw path.
- [x] Added the ninth CTest target, `terminal`.
- [x] Debug with diagnostics, sanitizer-enabled, warning-clean, and Release builds pass all nine CTest targets.
- [x] Pseudo-terminal checks pass for SIGWINCH, SIGTERM, SIGHUP, terminal settings restoration, and alternate-screen cleanup.

Next: Chapter 54.7 — Interactive Failure-Path Verification and Final Hardening Review.

## Chapter 54.7 — Interactive Failure-Path Verification and Final Hardening Review

- [x] Added `tests/test_pty_integration.py`, a POSIX pseudo-terminal integration test.
- [x] Registered the integration test with CTest when Python 3 is available; the test has a 20-second timeout.
- [x] Verified Ctrl-Q shutdown and restoration of terminal attributes, alternate-screen state, and wrap mode.
- [x] Verified the event loop survives a valid SIGWINCH resize.
- [x] Verified SIGINT, SIGTERM, and SIGHUP perform normal cleanup.
- [x] Verified a startup file-load failure returns status 1 after terminal cleanup.
- [x] Debug with diagnostics, AddressSanitizer/UndefinedBehaviorSanitizer, warning-clean, and Release configurations pass all ten CTest targets.

Chapter 54 hardening milestone complete. Deterministic allocation and terminal-write failure injection remain open follow-up work.

## Follow-up — Diagnostic Logging to `log.txt`

- [x] Added `src/common/logger.{h,c}` with ERROR/WARN/INFO/DEBUG levels, timestamps, component labels, and formatted records.
- [x] Added CMake option `TEDIT_ENABLE_LOGGING`; development configurations enable logging by default and Release omits application log calls.
- [x] Integrated lifecycle, input-event, command, terminal, rendering-failure, file-operation, recoverable-error, and Dirty Flag transition records.
- [x] Ensured log initialization/write failures are best-effort and do not become editor errors.
- [x] Each session writes a lifecycle startup record. The logger opens `log.txt` in write mode, replacing records from previous sessions; the startup record does include build-version and process-ID metadata.
- [x] Added `tests/test_logger.c` and registered the eleventh CTest target.
- [x] Extended pseudo-terminal integration to verify character input and Dirty Flag transition records, and verify that Release/logging-disabled builds do not create a log file.
- [x] Added `log.txt` and build directories to `.gitignore`.
- [x] Updated README with log location, configuration, and behavior.

- [x] Debug with diagnostics/logging, sanitizer-enabled, warning-clean, Release, and explicit logging-disabled configurations each pass all eleven CTest targets.
- [x] Verified the Release pseudo-terminal run does not create `log.txt`; development/Debug runs record lifecycle and failure events.
