# Current State

## Project

Terminal text editor implementation in C99 for Linux/POSIX, developed incrementally using CMake. The implementation repository is separate from the book repository and follows the architecture, software requirements, runtime model, coding standards, and ADRs recorded in `context/`.

## Current Milestone

**Chapter 49 — Navigating the Document: complete for the current ASCII-only scope**

Chapters 48 and 49 so far add logical-line splitting and merging, vertical Cursor movement with preferred-column preservation, and a two-offset Viewport for vertical and horizontal scrolling. The Renderer now draws the visible document region, and the event loop maintains Cursor visibility before redrawing. Enter (`0x0D`) and line feed (`0x0A`) split the current logical line at the Cursor. Backspace deletes the preceding character when the Cursor is inside a line; at column zero, it merges the current line into the previous line when one exists. The Cursor and Dirty Flag are updated after successful edits, and the screen is redrawn.

The build and registered CTest targets (`smoke` and `editing`) pass. Manual testing in WSL exposed two terminal-edge defects. First, full-width frame writes could interact with terminal auto-wrap and make the physical screen drift from the logical Viewport. The terminal runtime now disables auto-wrap while the editor is active and restores it during cleanup, and the Renderer validates Cursor coordinates before emitting terminal positioning sequences. Second, horizontal scrolling could make the application exit when a line reached the terminal's right edge. The application now initializes the Renderer with `columns - 1`, reserving the terminal's final physical column. The user confirmed that the horizontal edge issue is fixed in WSL. Regression tests cover repeated vertical/horizontal scrolling, invalid Renderer Cursor coordinates, and terminal-mode restoration.

## Completed

### Implementation Preparation

- Architecture and implementation decisions established.
- Implementation repository structure defined.
- CMake build structure established.
- Compiler and warning policy established.
- Development and Release build configurations established.
- Initial testing baseline established.
- Git workflow and implementation documentation structure established.

### Chapter 45 — Establishing the Runtime

- Terminal state management implemented.
- Original terminal attributes saved during initialization.
- POSIX raw terminal mode implemented.
- Terminal restoration implemented.
- Application lifecycle established.
- Blocking event loop introduced.
- `Ctrl-Q` provides the temporary exit command.
- EOF terminates the event loop when standard input reaches EOF.
- Cleanup occurs before application termination.
- Terminal management is isolated in the `terminal` module.
- Alternate-screen entry and exit are integrated into the terminal lifecycle.

### Chapter 46 — Building the First Editable Screen

#### Document

- Document represented as a vector of logical lines.
- Each `DocumentLine` owns a dynamically allocated character buffer.
- Document initialization creates one empty logical line.
- Document destruction releases allocated line and document storage.
- Document line access validates the requested row.
- Character insertion shifts existing bytes and updates line length.
- Document operations are implemented in the `document` module.

#### Cursor

- Cursor stores zero-based logical row and column coordinates plus a preferred column.
- Cursor initialization places it at the beginning of the Document.
- Cursor position assignment validates the target row and column and synchronizes the preferred column.
- Cursor advancement supports character insertion and synchronizes the preferred column.
- Up and Down movement preserve the preferred column while clamping the actual column to the destination line.
- Cursor operations are implemented in the `cursor` module.

#### Editor State

- `EditorState` owns the Document and Cursor.
- Editor State initialization establishes the initial editing state.
- Editor State destruction releases its owned Document resources.
- `EditorState` contains the `is_dirty` flag.
- Editor State acts as the central source of truth for the current editing session.

#### Screen Buffer

- Screen Buffer represents the visible terminal area as a contiguous allocation of `ScreenCell` objects.
- Each cell stores a character.
- Screen cells are addressed using logical row and column coordinates.
- Buffer initialization validates dimensions and allocation-size calculations.
- The buffer can be cleared, updated, queried, and destroyed.
- Screen Buffer storage is bounded by terminal dimensions.

#### Renderer

- Renderer constructs a complete frame from the current Editor State.
- The Screen Buffer is initialized one column narrower than the detected terminal width, reserving the terminal's final physical column to avoid right-edge pending-wrap behavior.
- Document characters are copied into the visible Screen Buffer using the Viewport offsets.
- Vertical and horizontal scrolling expose a different portion of the document without changing the Document.
- Content beyond the current screen dimensions is clipped.
- Terminal output uses ANSI cursor-positioning sequences.
- The terminal text cursor is hidden during frame rendering and restored afterward.
- Frame output is assembled in a reusable, dynamically allocated output buffer.
- The output buffer grows when necessary and is reused across subsequent frames.
- The output helper handles partial writes and retries interrupted writes.
- Rendering follows the full-redraw strategy.

### Chapter 47 — Editing Text

#### Document Editing

- `document_insert_character()` inserts a character at a valid row and column.
- `document_delete_character()` removes a character at a valid row and column.
- Insertion and deletion shift bytes as required and update line length.
- Invalid rows and columns are rejected without modifying the line.

#### Cursor Movement

- `cursor_move_left()` moves left within the current logical line and refuses to move before column zero.
- `cursor_move_right()` moves right within the current line and refuses to move beyond the line length.
- Horizontal movement across logical line boundaries is handled by the Chapter 48 editing behavior rather than by the horizontal Cursor movement functions.

#### Dirty Flag

- `is_dirty` initializes to `false`.
- Successful character insertion sets the flag to `true`.
- Successful character deletion sets the flag to `true`.
- The flag is not yet cleared by a save operation because file saving is not implemented.

#### Interactive Editing Loop

- `Ctrl-Q` exits the editor.
- EOF terminates the event loop.
- Printable ASCII characters are inserted at the current Cursor position.
- The Cursor advances after successful insertion.
- Backspace is recognized as byte `0x7F` or `0x08`.
- Left, Right, Up, and Down Arrow input is recognized using the current basic `ESC [` sequence handling.
- Successful movement and text edits update Cursor visibility through the Viewport and trigger a full redraw.
- Unsupported control characters and non-ASCII input bytes are ignored.
- Input handling remains temporarily inside `main.c`; the final Input Handler, Input Parser, Input Engine, and Command Engine architecture has not yet been integrated.

### Chapter 48 — Working with Lines

#### Document Line Operations

- `document_split_line(Document *document, size_t row, size_t column)` splits one logical line at a valid insertion position.
- The original line retains the prefix; a newly allocated line receives the suffix.
- Splitting at column zero creates an empty line before the original contents.
- Splitting at the end of a line creates an empty following line.
- `document_merge_lines(Document *document, size_t row)` appends line `row + 1` to line `row` and removes the second line.
- Merge rejects a row without a following line and checks for combined-length overflow and allocation failure.
- Document operations preserve the logical-line ordering and update `line_count` after successful changes.

#### Enter and Backspace

- Enter (`0x0D`) and line feed (`0x0A`) invoke line splitting at the current Cursor position.
- After a successful split, the Cursor moves to column zero of the new line.
- Backspace at a column greater than zero deletes the character immediately before the Cursor and moves the Cursor to the deleted character's former position.
- Backspace at column zero merges the current line into the previous line when the current row is greater than zero.
- After a successful merge, the Cursor moves to the previous line's former end position.
- Backspace at the beginning of the first line is a no-op.
- Successful line splitting, line merging, insertion, and deletion set `is_dirty` to `true` and trigger a redraw.

### Chapter 49 — Navigation and Viewport (complete for current ASCII-only scope)

- Cursor Up and Down use preferred-column behavior.
- `Viewport` stores `viewport_row` and `viewport_column` offsets.
- `viewport_ensure_cursor_visible()` adjusts both offsets to keep the Cursor within the Screen Buffer dimensions.
- Renderer maps screen coordinates to document coordinates using the Viewport offsets.
- The event loop updates the Viewport before redraws, including after insertion and Cursor movement.
- The previous terminal-width insertion restriction has been removed; horizontal scrolling now exposes later columns.
- The Screen Buffer uses `terminal_columns - 1` columns. The terminal's final physical column is intentionally reserved to avoid the right-edge behavior observed in WSL.
- Terminal auto-wrap is disabled during the editor session and restored during cleanup.
- Renderer cursor positioning is rejected when the Cursor lies outside the visible Screen Buffer, avoiding invalid screen-relative coordinate calculations.
- The current implementation accepts printable ASCII only, so logical and visual columns coincide. Tab expansion and Unicode display-width mapping are not yet implemented.
- Failed line operations do not proceed with Cursor or Dirty Flag updates.

#### Tests

- The `editing` test target covers Document editing, invalid operations, horizontal and vertical Cursor boundaries, preferred-column restoration, line splitting, line merging, empty-line behavior, and Cursor transitions after line edits.
- Viewport tests cover repeated vertical and horizontal scrolling, screen-boundary positions, invalid dimensions, and preservation of offsets on invalid calls.
- A Renderer integration test verifies that visible cells use Viewport offsets and that the terminal Cursor position is relative to the visible area.
- Both registered CTest targets (`smoke` and `editing`) pass.

## Current Runtime

The application currently follows this lifecycle:

```text
Program Start
      |
      v
Terminal Initialization
      |
      v
Enable Raw Mode
      |
      v
Enter Alternate Screen
      |
      v
Determine Terminal Dimensions
      |
      v
Initialize Editor State
      |
      v
Initialize Renderer and Screen Buffer
      |
      v
Render Initial Frame
      |
      v
Interactive Event Loop
      |
      +---- Ctrl-Q / EOF ----------------> Exit
      |
      +---- Printable ASCII ------------> Insert Character
      |                                      |
      |                                      v
      |                                 Advance Cursor
      |                                      |
      |                                      v
      |                                 Set Dirty Flag
      |                                      |
      |                                      v
      |                              Ensure Cursor Visible
      |                                      |
      |                                      v
      |                                 Render Frame
      |
      +---- Enter / Line Feed ----------> Split Current Line
      |                                      |
      |                                      v
      |                              Move to New Line, Column 0
      |                                      |
      |                                      v
      |                                 Set Dirty Flag
      |                                      |
      |                                      v
      |                              Ensure Cursor Visible
      |                                      |
      |                                      v
      |                                 Render Frame
      |
      +---- Backspace ------------------> Delete or Merge Lines
      |                                      |
      |                                      v
      |                              Update Cursor Position
      |                                      |
      |                                      v
      |                                 Set Dirty Flag
      |                                      |
      |                                      v
      |                              Ensure Cursor Visible
      |                                      |
      |                                      v
      |                                 Render Frame
      |
      +---- Left / Right Arrow ---------> Move Cursor Within Line
      |                                      |
      +---- Up / Down Arrow ------------> Move Cursor Between Lines
      |                                      |
      |                                      v
      |                              Ensure Cursor Visible
      |                                      |
      |                                      v
      |                                 Render if moved
      |
      v
Destroy Renderer
      |
      v
Destroy Editor State
      |
      v
Restore Terminal Settings
      |
      v
Leave Alternate Screen
      |
      v
Program Exit
```

If initialization fails, the application skips the event loop and cleans up only components whose initialization succeeded. Terminal restoration and alternate-screen exit are performed through the cleanup path after raw mode has been enabled.

## Current Source Structure

```text
tedit-main/
├── src/
│   ├── main.c
│   ├── viewport.c
│   ├── viewport.h
│   ├── terminal/
│   │   ├── terminal.c
│   │   └── terminal.h
│   ├── document/
│   │   ├── document.c
│   │   └── document.h
│   ├── cursor/
│   │   ├── cursor.c
│   │   └── cursor.h
│   ├── state/
│   │   ├── editor_state.c
│   │   └── editor_state.h
│   └── renderer/
│       ├── renderer.c
│       ├── renderer.h
│       ├── screen_buffer.c
│       └── screen_buffer.h
├── tests/
│   ├── test_smoke.c
│   └── test_editing.c
├── context/
├── CMakeLists.txt
└── README.md
```

## Verification Status

### Build and Automated Tests

- CMake configuration and build succeeded for the current Chapter 49 implementation.
- `tedit`, `test_smoke`, and `test_editing` build targets succeeded.
- CTest passed both registered tests: `smoke` and `editing`.
- The `editing` test covers line splitting and merging, Cursor boundaries and preferred-column behavior, Viewport scrolling, invalid dimensions, and Renderer/Viewport integration.
- Controlled pseudo-terminal checks exercised vertical and horizontal scrolling, including long lines, without the application exiting.

### Manual Verification

The user confirmed that vertical scrolling works after the auto-wrap and Cursor-coordinate fix. A further defect in WSL caused the application to close when a line reached the terminal's right edge. Reserving the terminal's final physical column fixed that issue; the user confirmed that the horizontal scrolling issue is now resolved. Broader end-to-end and failure-path validation remains part of later hardening.

## Architectural Boundaries

- **Terminal:** terminal configuration, raw mode, restoration, and alternate-screen lifecycle.
- **Document:** logical lines and character storage, including insertion, deletion, splitting, and merging.
- **Cursor:** logical editing position and horizontal movement within a line.
- **Editor State:** ownership and coordination of the Document and Cursor, including the Dirty Flag.
- **Screen Buffer:** in-memory representation of the visible frame.
- **Viewport:** tracks the first visible document row and visual column and adjusts offsets to keep the Cursor visible.
- **Renderer:** frame construction and terminal output; it remains a read-only consumer of Editor State and uses Viewport offsets when drawing the visible region.
- **File Manager:** complete-file POSIX reads, strict UTF-8 decoding, line-ending normalization, and failure-safe Document replacement.\n- **Application Event Loop:** temporary direct input handling and coordination of editing, Cursor movement, Dirty Flag updates, and rendering.

The Screen Buffer is presentation data and does not replace the Document as the authoritative text representation.

## Known Limitations

- Delete-key behavior is not implemented.
- Interactive input supports printable ASCII only; tab expansion and Unicode display-width mapping are not implemented.
- File loading is implemented for startup; saving is not implemented.
- The Dirty Flag is not yet connected to save behavior.
- Input Handler, Input Parser, Input Engine, and Command Engine integration is not implemented; input handling remains temporarily in `main.c`.
- Status Bar and Message Bar are not implemented.
- Terminal resizing and Screen Buffer reinitialization are not implemented.
- Incremental or dirty-cell rendering is not implemented.
- Allocation-failure paths and a broader terminal-size integration matrix need additional coverage.

## Current Focus

**Chapter 50 — Opening Files is implemented through Sections 50.1–50.8.** The File Manager loads and decodes files, zero-line Documents are supported by insertion, splitting, and rendering, Editor State resets after successful loading, and `main()` accepts an optional startup filename. The clean build and three registered CTest targets pass for this snapshot.

## Next Step

**Chapter 51 — Saving Files**

Implement UTF-8 serialization and the safe-save workflow through the File Manager. Keep tab expansion, Unicode display-width mapping, and terminal resize handling visible as limitations.
