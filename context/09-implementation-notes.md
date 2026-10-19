# Implementation Notes

## Volume X — Implementation Notes

Volume X is being implemented as a separate C99/Linux-POSIX implementation repository. The source code is the primary artifact, and implementation proceeds incrementally from the minimal runtime toward the complete terminal editor.

The implementation must remain consistent with the Book Bible, Software Requirements Specification, System Architecture, Runtime Model, Coding Standards, Project Structure, and Architecture Decision Records.

The implementation repository maintains four complementary context documents:

- `current-state.md` — concise snapshot of the current implementation.
- `implementation-status.md` — milestone progress and verification status.
- `implementation-notes.md` — important implementation discoveries, decisions, and reasoning.
- `known-issues.md` — unresolved implementation problems.

---

## Chapter 45 — Establishing the Runtime

Chapter 45 established the first executable runtime of the editor.

The implementation deliberately stopped at the runtime boundary. Editor-specific components were introduced in the next milestone rather than being mixed into terminal initialization.

### Terminal State

A dedicated `terminal` module owns terminal configuration.

The module maintains:

```text
TerminalState
├── original_attributes
├── initialized
└── raw_mode_enabled
```

The original terminal attributes are captured during initialization and retained until cleanup.

Raw mode is constructed from the saved attributes rather than modifying the saved copy directly. The `raw_mode_enabled` state is set only after the terminal successfully accepts the new attributes.

### Raw Mode

The runtime enables POSIX raw terminal behavior appropriate for interactive editing.

The implementation disables normal terminal processing required for editor input, including character echo and canonical input processing. This allows the application to receive input without waiting for a complete terminal line.

### Terminal Cleanup

Terminal restoration is explicitly part of the application lifecycle.

The application must not simply terminate after entering raw mode. It restores the original terminal attributes before exiting. Cleanup is also safe when initialization succeeded but raw mode was never successfully enabled.

The runtime invariant is:

```text
If raw mode was successfully enabled,
the application must restore the original terminal state
before normal termination.
```

This invariant applies to both normal exit and failure paths.

### Application Lifecycle

The runtime follows this sequence:

```text
Program Start
      |
      v
Terminal Initialization
      |
      v
Raw Mode Activation
      |
      v
Event Loop
      |
      v
Exit Condition
      |
      v
Terminal Restoration
      |
      v
Program Exit
```

Initialization failures prevent entry into the event loop. If raw-mode activation fails after terminal initialization, cleanup is still performed before termination.

### Minimal Event Loop

The initial event loop is intentionally temporary and minimal:

```text
Wait for one input byte
        |
        +-- Ctrl-Q --> Exit
        |
        +-- EOF ----> Exit
        |
        +-- Other --> Ignore
```

The loop uses a blocking `read()` and reads one byte at a time.

The direct `Ctrl-Q` check is not intended to become the final input architecture. It will later be replaced by the established:

```text
Input Handler
      |
      v
Input Parser
      |
      v
Input Engine
      |
      v
Command Engine
```

### Architectural Boundary

The terminal implementation remains at the platform-facing boundary. Terminal configuration and restoration do not require the runtime to know about Document, Cursor, Viewport, Renderer, or the future input-processing components.

This establishes the principle that platform lifecycle management should be reliable before editor behavior depends on it.

### Implementation Lesson

> Establish the application's lifecycle and ownership boundaries before adding editor behavior.

---

## Chapter 46 — Building the First Editable Screen

Chapter 46 introduced the first persistent editor state, a document representation, a basic Cursor, a Screen Buffer, and a Renderer. The application now supports a limited but working interactive editing loop.

The implementation was built on top of the existing terminal runtime rather than replacing it.

### Editor State as the Central State Model

`EditorState` owns the Document and Cursor:

```text
EditorState
├── Document
└── Cursor
```

This follows ADR-006: Editor State is the central source of truth for the application's editing state.

The Renderer receives a `const EditorState *`, making its read-only relationship to editor state explicit. Rendering should transform the current state into visible output rather than modifying the Document or Cursor.

### Document Representation

The Document uses a dynamically allocated vector of logical lines.

Each `DocumentLine` contains:

```text
DocumentLine
├── data
├── length
└── capacity
```

The Document itself maintains the line array, its current count, and its capacity.

The initial Document contains one empty line. This gives the editor a valid starting document without requiring a special empty-document case in basic rendering and insertion.

Line contents are stored as byte arrays with an explicit length. They are not maintained as conventional NUL-terminated C strings. Document operations and rendering must therefore respect the recorded length rather than relying on string functions that expect a terminating NUL byte.

Character insertion shifts existing bytes when necessary and increases the line length. The initial interactive editing path accepts printable ASCII, so the current byte-oriented implementation is sufficient for this milestone.

This is an initial representation, not the final Unicode editing model. UTF-8 decoding and the distinction between byte offsets and logical character positions remain future work.

### Cursor Ownership and Coordinates

The Cursor stores zero-based logical document coordinates:

```text
Cursor
├── row
└── column
```

Cursor operations validate positions against the Document. The Cursor does not own or modify document text; text changes remain Document responsibilities.

The current `cursor_advance()` operation supports movement after a successful character insertion. Full navigation, preferred-column behavior, and movement across logical lines are deferred to later milestones.

### Screen Buffer

The Screen Buffer is a reusable grid of `ScreenCell` values backed by one contiguous allocation.

Its logical dimensions are the terminal rows and columns. A cell can be addressed using:

```text
index = row * columns + column
```

The buffer provides operations to initialize and destroy its storage, clear its cells, set individual cells, and retrieve cells within bounds.

The Screen Buffer separates the representation of a frame from the act of writing terminal output. The Renderer can construct the visible frame before sending it to the terminal.

The buffer represents the current visible terminal grid, not the complete document. The Document remains the authoritative owner of text.

### Full-Redraw Rendering

The implementation follows ADR-002: full redraw remains the baseline rendering strategy.

The Renderer builds a frame from the current Editor State:

```text
Editor State
     |
     v
Build Screen Buffer
     |
     v
Encode Terminal Output
     |
     v
Write Frame
     |
     v
Position Terminal Cursor
```

The Screen Buffer is cleared to spaces before visible document content is copied into it. Document lines are clipped to the currently visible rows and columns.

This approach keeps rendering straightforward: the Renderer derives a complete visible frame from the current state instead of tracking every individual change to the screen.

The initial implementation does not yet include a Viewport abstraction. The current rendering behavior is therefore limited to the portion of the document that fits directly within the terminal dimensions.

### Reusable Output Buffer

The Renderer also maintains a reusable output buffer.

Instead of constructing and writing each screen row independently, it calculates the required output length, ensures sufficient capacity, and encodes the row-positioning escape sequences and cell contents into the buffer before writing the frame.

This reduces repeated output operations and avoids allocating a new output buffer for every frame when the existing allocation is sufficient.

The output-writing helper handles partial writes and interrupted system calls. A successful high-level frame write is not necessarily one operating-system `write()` call: the helper may need multiple calls to complete the output.

This is an output-efficiency improvement, not a change to the rendering strategy. The editor still performs full redraws rather than dirty-cell or incremental rendering.

### Terminal Cursor Visibility

Manual testing exposed a terminal cursor display glitch during interactive rendering.

The Renderer was changed to hide the terminal's text cursor before writing a frame and show it again afterward. The application then positions the cursor according to the editor's current Cursor state.

The intended sequence is:

```text
Hide Terminal Cursor
        |
        v
Write Screen Frame
        |
        v
Position Terminal Cursor
        |
        v
Show Terminal Cursor
```

The implementation attempts to restore cursor visibility even when a rendering operation fails. A failure while restoring visibility is also treated as a rendering failure.

The distinction between the two cursor concepts matters:

- **Editor Cursor** — logical row and column in the Document.
- **Terminal Cursor** — terminal emulator's text insertion position.

The Renderer translates the former into the latter. Hiding the terminal cursor is a presentation operation; it does not change the Editor State.

### Application Editing Loop

The current application loop connects the existing runtime to the new editing components:

```text
Read Input Byte
      |
      v
Check Exit Condition
      |
      v
Check Printable ASCII
      |
      v
Insert Character
      |
      v
Advance Cursor
      |
      v
Render Frame
      |
      v
Wait for Next Input
```

The implementation currently handles input directly in `main.c`. This is a temporary arrangement for the first editable-screen milestone, not the final input architecture.

The loop ignores unsupported bytes and exits on `Ctrl-Q`. It also prevents insertion from reaching the final terminal column because the implementation does not yet have a Viewport or horizontal-scrolling behavior.

The event loop redraws after successful character insertion. This keeps the initial interaction model simple while the input and command architecture remains unfinished.

### Resource Ownership and Cleanup

The new modules introduce additional dynamically allocated resources:

- Document line storage and the line array.
- Screen Buffer cell storage.
- Renderer output-buffer storage.

Each module provides initialization and destruction functions for its owned resources.

The application tracks successful initialization so cleanup can destroy only the components that were initialized. Terminal restoration remains part of the final cleanup path.

The general rule is:

> A component owns the resources it allocates and provides the operations required to release them.

This keeps memory management explicit and reduces the risk of double-free operations or cleanup of uninitialized resources.

### Verification and Evidence

The following Chapter 46 checks were reported as successful:

- The project builds successfully.
- The initial screen is displayed.
- Printable ASCII characters can be inserted.
- Repeated typing updates the visible document.
- The Cursor advances after insertion.
- The terminal cursor display glitch is resolved by the hide/show change.
- `Ctrl-Q` exits the application.
- Terminal settings are restored, and the shell remains usable afterward.

These are build and manual-verification results. They do not establish that comprehensive automated tests for Document, Cursor, Screen Buffer, and Renderer have passed.

### Current Limitations

The first editable screen remains intentionally incomplete.

The implementation does not yet provide:

- Backspace or Delete.
- Enter and logical-line splitting.
- Full Cursor navigation.
- Preferred-column navigation.
- Viewport-based scrolling.
- UTF-8-aware interactive input.
- File loading or saving.
- A Dirty Flag.
- Input Handler, Input Parser, Input Engine, or Command Engine integration.
- Status Bar or Message Bar.
- Terminal resize handling.
- Dirty-cell rendering.
- Comprehensive automated tests.

These are implementation gaps, not reasons to replace the working foundation.

### Implementation Lessons

Chapter 46 establishes several useful lessons:

1. **Build on the previous milestone.** New editor behavior should extend the terminal runtime without unnecessarily rewriting it.
2. **Keep document data authoritative.** The Screen Buffer is a rendered representation, not a second document model.
3. **Make ownership explicit.** Document, Screen Buffer, and Renderer each manage their own allocations.
4. **Separate rendering from state mutation.** The Renderer observes Editor State and generates output.
5. **Measure optimization at the correct layer.** Buffering terminal output reduces output overhead without requiring incremental rendering.
6. **Test terminal behavior in the actual environment.** A successful build alone cannot reveal all terminal cursor and restoration problems.
7. **Keep temporary code visibly temporary.** Direct input handling in the application loop must eventually move into the planned input architecture.

---

## Chapter 47 — Editing Text

Chapter 47 extends the editable-screen foundation with horizontal Cursor movement, Backspace deletion within a logical line, and a Dirty Flag. The implementation keeps the existing Document, Cursor, Editor State, Screen Buffer, Renderer, and event-loop structure. It does not yet introduce line splitting, line merging, or a dedicated input-processing pipeline.

### Horizontal Cursor Movement

The input loop recognizes the basic terminal escape sequences for Left and Right Arrow keys. These invoke the Cursor movement operations, which constrain movement to the current logical line. The Cursor does not move vertically or cross a logical-line boundary in this milestone.

This is deliberately a narrow navigation increment: the Cursor can move among character positions on a line, while document-wide navigation and Viewport behavior remain future work.

### Character Deletion and Backspace

The Document exposes a character-deletion operation that validates the requested logical row and column before modifying the line. For a valid position, bytes following the deleted character are shifted left and the line length is reduced.

Backspace is handled when the input byte is `0x7F` or `0x08`. If the Cursor column is greater than zero, the application deletes the byte immediately before the Cursor, places the Cursor at that character's former column, marks the Editor State dirty, and redraws the screen. At column zero, Backspace is ignored. It does not merge the current line with the preceding line.

The terminal Delete key is not implemented. It must not be described as supported merely because the Document now has a deletion operation: the current interactive path only wires deletion to Backspace.

### Dirty Flag

`EditorState` now contains an `is_dirty` flag initialized to `false`. Successful printable-ASCII insertion and successful Backspace deletion set the flag to `true`. Cursor movement alone does not mark the document dirty.

The flag records that editing has modified the in-memory document. File saving and clearing the flag after a successful save are not yet implemented, so the flag currently has no persistence workflow attached to it.

### Event-Loop Integration

Input recognition remains inside `main.c`. The current loop handles the limited set of supported operations directly:

```text
Read Input
    |
    +-- Ctrl-Q / EOF --> Exit
    |
    +-- Printable ASCII --> Insert --> Advance Cursor --> Mark Dirty --> Redraw
    |
    +-- Backspace --> Delete Previous Byte (if column > 0)
    |                  --> Move Cursor Left --> Mark Dirty --> Redraw
    |
    +-- Left / Right Arrow --> Move Cursor Within Line --> Redraw
    |
    +-- Other Input --> Ignore
```

This remains a temporary integration point. The planned Input Handler, Input Parser, Input Engine, and Command Engine have not been introduced, and unsupported input is ignored. The existing terminal-width limitation also remains: insertion cannot yet rely on Viewport-based horizontal scrolling.

### Verification and Evidence

For the Chapter 47 repository snapshot, the following checks succeeded:

- CMake configuration completed successfully.
- The project built successfully.
- The `smoke` automated test passed.
- The `editing` automated test passed.

These results confirm the recorded build and automated-test run for that snapshot; they do not establish comprehensive correctness. Manual verification of Left/Right Arrow behavior, Backspace boundaries, redraw behavior, and terminal cleanup after editing remains outstanding.

### Current Limitations

Chapter 47 does not yet provide:

- Delete-key handling.
- Enter or logical-line splitting.
- Backspace line merging at column zero.
- Vertical Cursor movement or full document navigation.
- Viewport-based scrolling.
- UTF-8-aware editing; the interactive insertion path remains printable ASCII and deletion is byte-oriented.
- File loading or saving.
- A save workflow that clears the Dirty Flag.
- The dedicated input-processing and command pipeline.
- Status Bar, Message Bar, terminal resize handling, or comprehensive test coverage.

These are known scope boundaries for the current milestone, not claims that the corresponding features have been tested and found defective.

### Implementation Lessons

1. **Wire behavior explicitly.** A Document deletion primitive does not imply that every deletion key is supported by the interactive application.
2. **Keep Cursor and Document responsibilities separate.** The Document changes line contents; the application coordinates the edit with Cursor placement, Dirty Flag updates, and redraw.
3. **Track mutations, not navigation.** In this milestone, insertion and successful deletion mark the document dirty; horizontal movement alone does not.
4. **Treat boundaries as defined behavior.** Backspace at column zero is currently a no-op; line merging belongs to the next milestone.
5. **Separate automated evidence from manual verification.** Passing the current smoke and editing tests does not replace testing the key sequences and terminal behavior interactively.
6. **Continue from the current source.** Chapter 48 should extend the existing Document and Cursor APIs with line operations instead of replacing the implementation wholesale.

---

## Chapter 48 — Working with Lines

Chapter 48 extends the vector-of-lines Document implementation with logical-line splitting and merging. It preserves the existing module boundaries and keeps input recognition in `main.c`; no new module or architecture change is introduced.

### Document Operations

Two Document operations are added:

- `document_split_line(Document *document, size_t row, size_t column)` splits a line at a valid insertion position. The original line retains the prefix, and a newly allocated line owns the suffix. Splitting at column zero creates an empty prefix line; splitting at the line length creates an empty suffix line.
- `document_merge_lines(Document *document, size_t row)` appends the next line's bytes to the selected line, releases the removed line's storage, shifts the remaining line records, and decreases the line count.

Both operations validate their row/column boundaries. Allocation failure must be reported without proceeding with the structural edit. The line vector remains the authoritative representation, consistent with ADR-013.

### Input-Loop Integration

The existing application event loop handles Enter (`0x0D`) and line feed (`0x0A`) by splitting at the current Cursor position. After a successful split, the Cursor moves to column zero of the new line.

Backspace now has two editing paths:

- At a column greater than zero, it deletes the byte immediately before the Cursor and moves the Cursor left.
- At column zero on a row after the first, it merges the current line into the preceding line and moves the Cursor to the former end of that preceding line.

Backspace at the beginning of the first line remains a no-op. The application sets the Dirty Flag and redraws after successful mutations; unsuccessful operations do not count as successful edits. Horizontal Cursor movement remains confined to the current line.

### Verification and Evidence

The Chapter 48 working copy was configured and built with CMake, and the `smoke` and `editing` CTest targets passed. The user subsequently confirmed that all tests passed and that the editor builds and works correctly in the terminal.

This evidence establishes the reported build, automated-test, and interactive checks for this milestone. It does not establish comprehensive coverage of all terminal implementations, allocation-failure paths, UTF-8 input, or the features that remain outside this milestone.

### Implementation Lessons

1. **Treat line edits as structural mutations.** Splitting and merging change the line vector as well as line contents, so line count and line ownership must be updated consistently.
2. **Prepare allocations before changing structure.** New line storage and any required line-array capacity must be available before the split mutates the existing Document.
3. **Keep Cursor coordination in the application layer.** Document operations change text; the event loop applies the established Cursor transition, Dirty Flag update, and redraw.
4. **Define boundary behavior explicitly.** Enter at either line boundary creates an empty line on the corresponding side. Backspace at column zero merges only when a previous line exists.
5. **Record evidence precisely.** Passing `smoke` and `editing` tests plus the user's interactive confirmation is useful verification, but it is not equivalent to exhaustive regression or failure-path testing.
6. **Preserve the existing architecture.** Chapter 48 extends the Document API and event loop without replacing the vector-of-lines model or prematurely introducing the dedicated input pipeline.

### Current Limitations After Chapter 48

The implementation still does not provide:

- Delete-key behavior.
- Vertical Cursor movement or preferred-column behavior.
- Viewport management and scrolling.
- UTF-8-aware interactive editing; insertion remains printable ASCII and editing is byte-oriented.
- File loading or saving, or a save workflow that clears the Dirty Flag.
- The dedicated Input Handler, Input Parser, Input Engine, and Command Engine pipeline.
- Status Bar, Message Bar, or terminal resize handling.
- Comprehensive automated coverage of rendering, terminal integration, allocation failures, and other failure paths.

These are remaining scope boundaries, not claims that the corresponding features were tested and found defective.

---

## Chapter 49 — Navigating the Document

Chapter 49 adds document-wide navigation for the current printable-ASCII editing model. It extends the existing Cursor and Renderer behavior rather than replacing the established Document representation or full-redraw rendering pipeline.

### Vertical Cursor Movement and Preferred Column

The Cursor now tracks `preferred_column` in addition to its logical `row` and `column`.

- Left and Right movement establish the preferred column from the new actual column.
- Up and Down movement change the logical row while retaining the preferred column.
- When the destination line is shorter than the preferred column, the actual column is clamped to that line's length.
- Moving to a later line that is long enough restores the preferred column.
- Cursor repositioning after editing must keep the preferred-column state consistent with the resulting position.

This follows ADR-014. The Cursor continues to store logical document coordinates; it does not store terminal screen coordinates.

### Viewport and Scrolling

A dedicated Viewport stores two offsets:

```text
Viewport
├── viewport_row
└── viewport_column
```

`viewport_row` identifies the first visible logical document row. `viewport_column` identifies the first visible display column in the current ASCII-only renderer. The Viewport does not duplicate or modify Document content.

Before rendering, the application maintains Cursor visibility by adjusting the Viewport offsets when the Cursor would otherwise fall outside the visible region. The Renderer uses those offsets to draw the visible document region and to translate the logical Cursor position into terminal-relative coordinates.

The current interactive input path supports printable ASCII only. Under that restriction, a logical column and a displayed column coincide. Tabs and Unicode display-width mapping are not implemented, so this implementation must not be treated as the final variable-width character model described by ADR-019.

### Event-Loop Integration

The existing event loop in `main.c` recognizes Up and Down Arrow escape sequences and delegates movement to the Cursor module. Successful movement triggers a redraw. The application also updates the Cursor's position after line edits so preferred-column state remains consistent.

The input-processing pipeline remains temporary and direct: the dedicated Input Handler, Input Parser, Input Engine, and Command Engine have not yet been integrated.

### Terminal Edge Behavior and WSL Fix

Manual testing in WSL exposed a crash when typing a long line as it reached the terminal's rightmost physical column. The final fix initializes the Renderer with one fewer column than the terminal reports, reserving the final physical column.

This is a terminal-compatibility workaround for the observed right-edge wrapping behavior. It reduces the usable rendering width by one column; it does not change the Document's logical line length or the Cursor's logical coordinate model. Do not describe the workaround as a general solution for all terminal emulators without further testing.

An earlier vertical-scrolling issue was also found during manual testing: after scrolling to the bottom, Up movement initially appeared unresponsive and subsequent edits could affect a different displayed row. The Viewport/Renderer integration was corrected, and the user confirmed that vertical scrolling now behaves correctly.

### Verification and Evidence

For the current Chapter 49 implementation:

- CMake configuration and build succeeded.
- The registered `smoke` and `editing` CTest targets passed.
- Regression tests cover vertical movement, preferred-column restoration, Viewport boundaries, repeated scrolling, and Renderer positioning.
- The user confirmed that the vertical scrolling issue is resolved in the terminal.
- The user confirmed that the horizontal right-edge crash is resolved in WSL after the final-column reservation.

The tests and user-reported interactive checks are evidence for the current supported scope. They do not establish Unicode-aware rendering, terminal resizing, comprehensive allocation-failure behavior, or compatibility with every terminal emulator.

### Implementation Lessons

1. **Keep coordinate spaces distinct.** Cursor coordinates are logical document positions; Viewport offsets describe the visible region; Renderer calculations map between document and terminal positions.
2. **Preserve preferred-column intent.** Vertical movement should clamp the actual column on short lines without discarding the preferred column.
3. **Update visibility after state changes.** Cursor movement and editing must be followed by Viewport visibility maintenance before rendering.
4. **Test in the actual terminal environment.** Passing unit tests did not reveal the original scrolling symptoms; interactive testing in WSL exposed them.
5. **Reserve the terminal edge deliberately.** The final physical column is unused to avoid the observed right-edge failure. This is a compatibility trade-off and should remain documented.
6. **Do not overstate test coverage.** Passing the current two CTest targets and the user's manual verification does not replace broader end-to-end, resize, Unicode, or failure-path testing.
7. **Continue from the current source.** Chapter 50 should add file loading through the File Manager boundary while preserving the working Cursor, Viewport, Document, and Renderer modules.

### Current Limitations After Chapter 49

The implementation still does not provide:

- File loading or saving, or a save workflow that clears the Dirty Flag.
- UTF-8-aware interactive input and Unicode display-width mapping; insertion remains printable ASCII and editing is byte-oriented.
- Tab expansion.
- The terminal Delete key; Backspace is supported.
- The dedicated Input Handler, Input Parser, Input Engine, and Command Engine pipeline.
- Status Bar or Message Bar.
- Terminal resize handling and frame reinitialization.
- Comprehensive automated coverage of rendering, terminal integration, allocation failures, and all failure paths.

These are remaining scope boundaries, not claims that the corresponding features were tested and found defective.

---

## Previous Milestone — Chapter 50: Opening Files

Continue from the verified Chapter 49 source. Implement file loading through the dedicated File Manager boundary. Keep file-system operations out of the Document, Cursor, Viewport, and Renderer modules. Follow the project's established encoding and line-ending decisions, and add focused tests for successful loading, empty files, invalid paths, and read failures before integrating the load operation into application initialization.


---

## Chapter 50 — Opening Files (Sections 50.1–50.8)

The File Manager reads a complete file into a byte buffer, validates and decodes UTF-8, and constructs the Document through the existing Document API. LF and CRLF are normalized to logical line boundaries. The destination Document is replaced only after the entire file has been read and decoded successfully.

An empty file is represented by a Document with zero logical lines, distinct from a new unsaved Document, which contains one empty line. The Document API therefore supports two zero-line transitions: insertion at `(0, 0)` creates the first line containing the inserted code point, and splitting at `(0, 0)` creates two empty lines. Both transitions build their required allocations before publishing the new line count.

The Renderer already builds each frame by clearing the Screen Buffer and skipping document rows outside `line_count`; a regression test now confirms that a zero-line Document renders as a blank area without a synthetic line. The Editor State exposes `editor_state_load_file()`, which uses the File Manager's failure-safe load contract and resets Cursor, Viewport, and the Dirty Flag only after success.

`main()` accepts either no filename or one filename. A requested file is loaded after Editor State initialization but before Renderer initialization and the initial frame. Startup load failure follows the common cleanup path, restoring terminal state and leaving the alternate screen before reporting a diagnostic to standard error.

Verification performed for this snapshot: CMake configuration/build succeeded and all registered CTest targets (`smoke`, `editing`, and `file`) passed. Interactive testing of startup in a real terminal remains a manual check.

## Chapter 51 — Save Integration (Sections 51.2–51.3)

`file_save_document()` serializes the Document at the File Manager boundary. It encodes Unicode code points to UTF-8, rejects surrogate values and values above U+10FFFF, appends LF after every logical line, and emits no bytes for a zero-line Document. The implementation writes to a `mkstemp()` file using a suffix appended to the destination path, buffers output, retries interrupted writes, handles partial writes, checks `close()`, and calls `rename()` only after the full output succeeds. It unlinks the temporary path after a failure where possible. `fsync()` durability, destination metadata preservation, and crash recovery for orphaned temporary files remain outside the baseline scope.

`editor_state_save_file()` is the Editor State coordination boundary. It returns `false` for invalid arguments or a File Manager error and leaves `is_dirty` unchanged. It sets `is_dirty = false` only after `file_save_document()` returns `FILE_SAVE_OK`. Tests verify success, invalid arguments, a serializer failure that leaves an existing destination untouched, and zero-line serialization. The Event Loop has not yet been connected to this API.

Current verification: configure/build succeeds; `smoke`, `editing`, `file`, and `file_save` pass under CTest.


### Chapter 51 — Section 51.4: Event Loop Integration

The current direct input loop recognizes Ctrl-S (`0x13`). The optional startup filename is passed into `application_event_loop()`. When it is non-NULL, Ctrl-S calls `editor_state_save_file()` with that path. The result is intentionally not used to terminate the loop: failed saves preserve the Dirty Flag through the Editor State API and the user can continue editing.

This integration intentionally does not implement Save As for a new unnamed Document. It also does not print status text directly from `main.c`; doing so would create a competing terminal-output path before the Message Bar exists. Visible success/failure feedback is deferred to the interface chapter.

Verification: CMake configuration/build succeeded and all four CTest targets (`smoke`, `editing`, `file`, `file_save`) passed. A pseudo-terminal check launched the application with a file, typed `X`, sent Ctrl-S and Ctrl-Q, then verified the destination contained `Xoriginal\n`.


### Chapter 51 — Section 51.5: Failure-Path Review

Added a regression case for the atomic replacement failure path. The test creates a directory at the requested destination path, then asks `editor_state_save_file()` to save there. Temporary-file creation and serialization can succeed, but `rename()` must fail when replacing the directory. The test verifies the call reports failure, `is_dirty` remains true, the in-memory character remains unchanged, the directory remains present, and no sibling temporary file with the generated `.tedit-` prefix remains in `/tmp`.

The earlier invalid-code-point test continues to exercise a serialization failure and verifies that an existing destination file remains byte-for-byte unchanged. These tests do not simulate kernel-level write or close failures; those paths remain guarded in the implementation but are not directly injected by the current test harness. `fsync()` durability and destination metadata preservation remain explicitly deferred by the baseline design.

Verification for Section 51.5: the standard CMake configure/build succeeded; a separate build with `-Wall -Wextra -Wpedantic -Werror` succeeded; `smoke`, `editing`, `file`, and `file_save` passed in both builds. Pseudo-terminal checks verified both successful Ctrl-S persistence and that a replacement failure does not terminate the Event Loop; the failure case also confirmed temporary-file cleanup. Chapter 51 is complete for the current existing-file save workflow. The next milestone is Chapter 52, where the Message Bar should provide visible save feedback without bypassing the Renderer/terminal output path.

## Chapter 52.5 — Input Engine and Command Dispatch

The Input Engine translates `InputEvent` values into `EditorCommand` values without reading terminal bytes or mutating Editor State. The Command Engine executes editing, cursor movement, save, and quit commands against Editor State and reports whether the Event Loop should redraw or quit. The Event Loop now orchestrates Input Handler → Input Parser → Input Engine → Command Engine → optional Renderer redraw. Save feedback remains routed through Editor State and the Renderer; Save As is still deferred.

Verification: Debug and warning-clean builds passed all seven CTest targets (`smoke`, `editing`, `file`, `file_save`, `input_parser`, `input_engine`, `command_engine`). A pseudo-terminal integration check verified insertion, Ctrl-S persistence, and Ctrl-Q exit. Next is Chapter 52.6 — integration review and key-binding boundary.


## Chapter 52 — Section 52.7: Filename Entry and Save As

Ctrl-S now has two paths. If `EditorState.file_path` exists (or a startup save path is available), it saves to the associated path. If the document is unnamed, it activates bounded filename entry. The Status Bar changes to a `Save as:` prompt, and the Renderer positions the terminal cursor after the entered path. Printable input is encoded into UTF-8 bytes, Backspace removes the last encoded code point, Enter attempts the save, and standalone Escape cancels after the Input Handler's Escape timeout.

`editor_state_save_as()` copies the candidate path before invoking `file_save_document()`. Only after the File Manager reports success does Editor State replace `file_path` and clear `is_dirty`. On failure, the filename prompt remains active so the user can correct the path or cancel; the existing file association, Document, and Dirty Flag remain unchanged. Cancellation clears only the transient filename-entry state and updates the Message Bar. The Command Engine ignores unrelated commands while the filename prompt is active.

Verification: all eight CTest targets pass in Debug and with `-Wall -Wextra -Wpedantic -Werror`. Pseudo-terminal checks verified entering a path, saving `x\n`, exiting normally, and canceling filename entry with Escape. Known presentation limitation: non-ASCII filename bytes are rendered as `?` in the prompt/status row, although the UTF-8 path bytes are retained for filesystem operations.


## Chapter 52.8 — Interface Refinement and Unsaved-Changes Confirmation

The final screen layout uses `rows - 3` document rows, one blank separator row, the Message Bar at `rows - 2`, and the Status Bar at `rows - 1`. The Renderer applies ANSI `30;47` rendition to the entire Message Bar row and sets the same rendition before placing the cursor in filename-entry mode. Other rows begin with a rendition reset. Filename entry moved from the Status Bar to the Message Bar, so the Renderer chooses a short prompt prefix based on the current entry error and positions the cursor after that prefix plus `filename_entry_cursor`.

`EditorState` now tracks `filename_entry_cursor`, `quit_confirmation_active`, and `quit_after_save`. Filename edits insert/delete complete UTF-8 code points at byte offsets. The Input Parser recognizes the common Delete sequence `ESC [ 3 ~`; the Input Engine maps it to `EDITOR_COMMAND_DELETE_CHARACTER`, which is acted upon only while filename entry is active. Document Delete behavior remains deferred to the earlier navigation/editing milestone.

Ctrl-Q checks the Dirty Flag. A clean document exits immediately. A dirty document enters a confirmation state: `Y` routes through the existing save path, `N` quits without saving, and Escape cancels. If there is no associated path, choosing `Y` activates filename entry and records `quit_after_save`; only successful saving causes the Event Loop to exit. Save failure leaves the editor open and the Dirty Flag set. The existing safe-save and atomic replacement implementation is unchanged. The user explicitly chose not to add a separate Save As command for named documents; Ctrl-S keeps its existing meaning.

Verification: Debug and warning-clean builds pass all eight CTest targets. Pseudo-terminal checks verified filename insertion/movement/Delete and save, white-background/black-foreground Message Bar output, and dirty-quit confirmation, Escape cancellation, and discard.


## Chapter 53.2 — Terminal Sequences for Additional Keys

The Input Parser now recognizes Home, End, Page Up, Page Down, and Tab without exposing raw terminal sequences to the rest of the editor. Home accepts CSI `H`, `1~`, and `7~`; End accepts CSI `F`, `4~`, and `8~`. Page navigation accepts CSI `5~` and `6~`, while Tab is emitted from byte `0x09`. Existing Delete parsing (`CSI 3~`) is retained.

The parser tests cover each supported form. The Debug build and all eight CTest targets pass. This section deliberately changes only input recognition; command mapping and behavior will be implemented in subsequent sections.


## Chapter 53.3 — Home and End Navigation

Added `cursor_move_home()` and `cursor_move_end()` to the Cursor module. Home sets the current logical column and preferred column to zero. End uses the current `DocumentLine.length`, which is measured in Unicode code points rather than UTF-8 bytes. Both operations leave the row unchanged and report whether the position changed. The Input Engine maps `INPUT_EVENT_HOME` and `INPUT_EVENT_END` to explicit navigation commands, and the Command Engine executes them against the current Cursor. Page Up, Page Down, and Tab are recognized as input events but intentionally remain no-op commands until their own implementation sections.

Regression coverage now includes empty-line and boundary behavior, preferred-column updates, command-level redraw decisions, event-to-command mapping, and Home/End sequences through the parser-to-command pipeline. Verification: Debug build and all eight CTest targets pass.


## Chapter 53.4 — Delete-Key Editing

Connected `EDITOR_COMMAND_DELETE_CHARACTER` to document editing. When the Cursor is before the current logical line's end, the Command Engine calls `document_delete_character()` at the Cursor's logical code-point column. When the Cursor is at line end and a following line exists, it calls `document_merge_lines()` on the current row and keeps the Cursor at the join point. At the end of the final line, Delete is a no-op.

The Cursor is revalidated with `cursor_set_position()` after a successful mutation, resetting the preferred column to the current logical column. The Dirty Flag and redraw result change only after a successful deletion or merge. Document operations remain responsible for allocation and storage; the Command Engine coordinates the Cursor and Editor State. Filename-entry Delete behavior is unchanged.

Regression tests cover deleting an ASCII character, deleting before a multi-byte Unicode code point, joining adjacent lines, preserving the join position, and doing nothing at the final document boundary. The input-pipeline test verifies `ESC [ 3 ~` reaches document editing. Debug and warning-clean builds pass all eight CTest targets.


## Chapter 53.5 — Page Up and Page Down Navigation

`Cursor` now provides `cursor_move_page_up()` and `cursor_move_page_down()`. These operations reuse the existing vertical-movement helper, so the cursor moves by a requested number of logical rows while preserving `preferred_column`; a shorter destination line clamps the actual column without losing the preferred value. Movement clamps to the first and last Document rows and returns false when no position change is possible.

`EditorCommand` now includes `page_rows`, used only by page-navigation commands. The Event Loop supplies the current `Renderer.document_rows`, which reflects the visible document area rather than the total terminal height. The Input Engine maps Page Up and Page Down to their commands, and the Command Engine invokes the Cursor operations. Page movement does not edit text or change the Dirty Flag.

Regression coverage exercises page-sized movement, top/bottom boundaries, short-line clamping and preferred-column recovery, redraw behavior, and `ESC [ 5 ~` / `ESC [ 6 ~` through the complete input pipeline. Verification: a Debug build with `-Wall -Wextra -Wpedantic -Werror` succeeds and all eight CTest targets pass.


## Chapter 53.6 — Tab Insertion and Configured Tab Width

`EditorState` now owns an `EditorConfiguration` value whose `tab_width` is initialized to `DEFAULT_TAB_WIDTH` (4). This milestone introduces the built-in setting and its consumer; it does not add configuration-file parsing or command-line overrides.

The Input Engine maps `INPUT_EVENT_TAB` to `EDITOR_COMMAND_INSERT_TAB`, rather than treating Tab as a generic printable character. The Command Engine inserts U+0009 into the Document as one logical code point. Because this is a dedicated command, filename-entry mode does not append a tab to a path. The existing Document and Cursor APIs continue to count logical code points, so insertion advances the Cursor by one logical column and marks the Document dirty only on success.

The Renderer expands each U+0009 into spaces through the next tab stop. A public `renderer_cursor_visual_column()` helper maps the current logical Cursor position to a visual display column. The Event Loop uses that result when ensuring Cursor visibility; the Renderer uses it for bounds validation and final terminal cursor placement. `Viewport.viewport_column` therefore represents a visual column, while `Cursor.column` remains a logical code-point column. Other Unicode code points still occupy one display cell in this milestone; full Unicode display-width handling remains in Sections 53.7–53.8.

Tests cover the default tab width, a non-default width, visual cursor mapping, spaces in the Screen Buffer, Tab event-to-command mapping, insertion through the complete input pipeline, and the existing regression suite. Verification: Debug and warning-clean builds (`-Wall -Wextra -Wpedantic -Werror`) pass all eight CTest targets. A pseudo-terminal check inserted `a`, Tab, and `b`, saved with Ctrl-S, and exited with Ctrl-Q; the saved file contained `a\tb\n`.


## Chapter 53.7 — UTF-8 Keyboard Input and Character-Boundary Editing

The Input Parser accumulates UTF-8 lead and continuation bytes and emits `INPUT_EVENT_CHARACTER` only when a complete valid Unicode scalar value has been decoded. Tests now cover two-byte U+00E9, three-byte U+20AC, and four-byte U+1F600 input. Invalid leading bytes, overlong sequences, UTF-16 surrogate values, values above U+10FFFF, interrupted sequences, and timeouts do not produce partial character events. If a non-continuation byte arrives during a sequence, the incomplete scalar is discarded and that byte is parsed again as a possible independent key.

The Document stores `uint32_t` code points in each logical line; `DocumentLine.length` and `Cursor.column` are logical code-point counts, not UTF-8 byte offsets. The Command Engine's Backspace and Delete operations call Document operations at these logical positions. Added tests prove that a four-byte UTF-8 sequence inserts one code point and that Backspace removes it in one operation, updating the Cursor by one logical column. Existing Delete tests cover deletion of a multi-byte character at the cursor.

Verification: configured a fresh Debug build with `-Wall -Wextra -Wpedantic -Werror`, compiled all targets, and passed all eight CTest targets. This section verifies input decoding and logical editing boundaries; it does not yet implement terminal display-width calculations for wide or combining characters, which are addressed by Section 53.8.


## Chapter 53.8 — Logical Cursor Positions and Visual Display Columns

- Kept `Cursor.column` and Document line lengths in logical Unicode code points.
- Added display-width mapping in the Renderer: tabs advance to the next configured tab stop, wide characters occupy two screen cells, and combining marks attach to the prior cell without advancing the visual column. A leading combining mark uses a dotted-circle base for visible rendering.
- Extended `ScreenCell` with a wide-character continuation marker and dynamically stored combining code points. Screen-buffer clearing resets visible mark counts while retaining allocations for reuse; destruction releases those allocations.
- Uses POSIX `wcwidth()` in a Unicode-capable locale and fallback ranges for common combining and wide Unicode blocks in the C/POSIX locale. This fallback is intentionally not represented as a complete Unicode width database.
- Updated the renderer cursor mapping and retained the existing Viewport interface, which receives visual display columns.
- Added a regression test for CJK wide width, combining-mark attachment, terminal UTF-8 output, and cursor positioning.


## Chapter 53.9 — Input Engine and Command Engine Integration

The integration audit confirmed the existing production path remains layered: the Input Parser recognizes raw terminal bytes and emits abstract events, the Input Engine translates each event to an `EditorCommand`, the Event Loop supplies the visible page height for page-navigation commands, and the Command Engine performs state changes. No production-code change was required for this milestone; the remaining gaps were integration-level regression coverage.

`tests/test_input_engine.c` now asserts that `page_rows` is initialized to zero by translation. This keeps terminal geometry out of the Input Engine; `src/main.c` remains responsible for setting `command.page_rows = renderer->document_rows` immediately before execution. `tests/test_input_pipeline.c` verifies that the alternate Home and End CSI parameter sequences reach the Cursor commands, and that Tab, Home, End, Page Up, and Page Down are ignored while filename entry is active. The latter test also exercises the parser timeout → Escape event → Cancel command path.

Verification: configured and built Debug and warning-clean configurations with `-Wall -Wextra -Wpedantic -Werror`; all eight CTest targets pass in both. A pseudo-terminal run exercised printable input, alternate Home/End, Page Up/Down, Tab, and dirty-quit discard; the editor exited successfully and restored the alternate screen. No new unresolved implementation issue was identified.


## Chapter 53.10 — Boundary, Regression, and Pseudo-Terminal Tests

Added `test_delete_boundaries_through_pipeline()` to `tests/test_input_pipeline.c`. The test sends the terminal Delete sequence (`ESC [ 3 ~`) through the Input Parser, Input Engine, and Command Engine. At the end of the final empty line, Delete leaves the Document clean and unchanged and requests no redraw. After Enter creates a second empty line, moving to the first line and pressing Delete merges the following line, leaves the Cursor at the join point, sets the Dirty Flag, and requests redraw.

The existing test suite also covers Home/End boundaries, Page Up/Down clamping and preferred-column recovery, Unicode scalar input and code-point deletion, configured Tab display columns, zero-line Document behavior, viewport boundary validation, and filename-entry isolation. No production-code change was needed in this section.

Verification performed from a fresh build directory: Debug CMake build and all eight CTest targets passed. A separate warning-clean build using `-Wall -Wextra -Wpedantic -Werror` also passed all eight targets. A pseudo-terminal session sent `abc`, Home, Delete, End, Tab, Ctrl-S, a temporary filename, Enter, and Ctrl-Q. The saved file contained exactly `bc\t\n`; the process exited successfully and emitted the alternate-screen restoration sequence.


## Chapter 53.11 — Resulting Implementation State

No production source changes were required for this final recording section. The completed implementation state was consolidated across the project records after checking the current repository snapshot.

### Chapter 53 capability summary

- Input Parser recognizes Home, End, Page Up, Page Down, Tab, and Delete sequences while retaining arrow, Backspace, Enter, and control-key handling.
- Input Engine maps recognized events to typed commands; the Event Loop supplies the visible page height for page navigation; the Command Engine applies Cursor movement and document edits.
- Home/End operate on logical line boundaries. Page movement uses the visible document height, clamps to Document boundaries, and preserves the preferred logical column.
- Delete removes one code point at the Cursor or merges the next logical line when positioned at the end of a non-final line. A Delete at the final Document boundary is a no-op.
- Tab inserts U+0009 as one logical code point. The Renderer expands it to the next configured tab stop; the built-in width is four columns.
- UTF-8 keyboard sequences are decoded into Unicode scalar values. Document line lengths and Cursor columns count code points rather than UTF-8 bytes.
- Renderer maps logical positions to visual columns and handles tabs, wide code points, and combining marks. The Viewport uses visual columns for horizontal visibility.
- Filename-entry mode remains isolated from document-only navigation and editing commands.

### Final verification

A fresh Debug build was configured with `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`, built successfully, and all eight CTest targets passed. A separate build configured with `-Wall -Wextra -Wpedantic -Werror` also built successfully and passed all eight CTest targets. A pseudo-terminal run sent `abc`, Home, Delete, End, Tab, Ctrl-S, a filename, Enter, and Ctrl-Q. The saved file contained exactly `bc\t\n`; the editor exited with status zero and emitted the alternate-screen restoration sequence.

### Scope boundary

Chapter 53 completes the established keyboard editing and navigation baseline; it does not claim terminal resize handling, allocation-failure hardening, a complete Unicode grapheme-width database, non-ASCII filename rendering, configuration-file parsing or command-line configuration overrides, or incremental rendering. These remain recorded as limitations for subsequent hardening.
