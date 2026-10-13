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

## Next Milestone — Chapter 50: Opening Files

Continue from the verified Chapter 49 source. Implement file loading through the dedicated File Manager boundary. Keep file-system operations out of the Document, Cursor, Viewport, and Renderer modules. Follow the project's established encoding and line-ending decisions, and add focused tests for successful loading, empty files, invalid paths, and read failures before integrating the load operation into application initialization.


---

## Chapter 50 — Opening Files (Sections 50.1–50.8)

The File Manager reads a complete file into a byte buffer, validates and decodes UTF-8, and constructs the Document through the existing Document API. LF and CRLF are normalized to logical line boundaries. The destination Document is replaced only after the entire file has been read and decoded successfully.

An empty file is represented by a Document with zero logical lines, distinct from a new unsaved Document, which contains one empty line. The Document API therefore supports two zero-line transitions: insertion at `(0, 0)` creates the first line containing the inserted code point, and splitting at `(0, 0)` creates two empty lines. Both transitions build their required allocations before publishing the new line count.

The Renderer already builds each frame by clearing the Screen Buffer and skipping document rows outside `line_count`; a regression test now confirms that a zero-line Document renders as a blank area without a synthetic line. The Editor State exposes `editor_state_load_file()`, which uses the File Manager's failure-safe load contract and resets Cursor, Viewport, and the Dirty Flag only after success.

`main()` accepts either no filename or one filename. A requested file is loaded after Editor State initialization but before Renderer initialization and the initial frame. Startup load failure follows the common cleanup path, restoring terminal state and leaving the alternate screen before reporting a diagnostic to standard error.

Verification performed for this snapshot: CMake configuration/build succeeded and all registered CTest targets (`smoke`, `editing`, and `file`) passed. Interactive testing of startup in a real terminal remains a manual check.
