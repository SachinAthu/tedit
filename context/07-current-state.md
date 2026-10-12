# Current State

## Project

Terminal text editor implementation in C99 for Linux/POSIX, developed incrementally using CMake. The implementation repository is separate from the book repository and follows the architecture, software requirements, runtime model, coding standards, and ADRs recorded in `context/`.

## Current Milestone

**Chapter 48 — Working with Lines: implemented and verified**

Chapter 48 adds logical-line splitting and merging to the existing editor. Enter (`0x0D`) and line feed (`0x0A`) split the current logical line at the Cursor. Backspace deletes the preceding character when the Cursor is inside a line; at column zero, it merges the current line into the previous line when one exists. The Cursor and Dirty Flag are updated after successful edits, and the screen is redrawn.

The user has confirmed that the editor builds, works in the terminal, and all tests pass. The current registered CTest targets are `smoke` and `editing`.

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

- Cursor stores zero-based logical row and column coordinates.
- Cursor initialization places it at the beginning of the Document.
- Cursor position assignment validates the target row and column.
- Cursor advancement supports character insertion.
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
- Document characters are copied into the visible Screen Buffer.
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
- Left and Right Arrow input is recognized using the current basic `ESC [` sequence handling.
- Successful movement and text edits trigger a full redraw.
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
- Failed line operations do not proceed with Cursor or Dirty Flag updates.

#### Tests

- The `editing` test target covers Document editing, invalid operations, horizontal Cursor boundaries, line splitting, line merging, empty-line behavior, and line-editing Cursor transitions.
- The `smoke` test target remains registered and passes.

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
      |                                 Render Frame
      |
      +---- Left / Right Arrow ---------> Move Cursor Within Line
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
├── docs/
│   └── chapter-48.md
├── context/
├── CMakeLists.txt
└── README.md
```

## Verification Status

### Build and Automated Tests

The user reports that the current Chapter 48 repository builds successfully, the editor works in the terminal, and all tests pass.

- CMake configuration and build succeeded.
- `tedit`, `test_smoke`, and `test_editing` build targets succeeded.
- CTest passed both registered tests: `smoke` and `editing`.
- The `editing` test covers line splitting and merging, boundary cases, empty-line behavior, and Cursor transitions in addition to the earlier editing checks.

### Manual Verification

The user has confirmed that the editor builds and works correctly in the terminal after Chapter 48. This includes successful interactive operation at the milestone level. The project still needs broader end-to-end and failure-path validation during the later hardening milestone.

## Architectural Boundaries

- **Terminal:** terminal configuration, raw mode, restoration, and alternate-screen lifecycle.
- **Document:** logical lines and character storage, including insertion, deletion, splitting, and merging.
- **Cursor:** logical editing position and horizontal movement within a line.
- **Editor State:** ownership and coordination of the Document and Cursor, including the Dirty Flag.
- **Screen Buffer:** in-memory representation of the visible frame.
- **Renderer:** frame construction and terminal output; it remains a read-only consumer of Editor State.
- **Application Event Loop:** temporary direct input handling and coordination of editing, Cursor movement, Dirty Flag updates, and rendering.

The Screen Buffer is presentation data and does not replace the Document as the authoritative text representation.

## Known Limitations

- Delete-key behavior is not implemented.
- Cursor movement with Up and Down Arrow keys is not implemented.
- Preferred-column behavior is not implemented.
- Viewport management and horizontal/vertical scrolling are not implemented; insertion and visibility remain constrained by the current terminal-width behavior.
- Interactive input supports printable ASCII only; UTF-8-aware input is not implemented.
- File loading and saving are not implemented.
- The Dirty Flag is not yet connected to save behavior.
- Input Handler, Input Parser, Input Engine, and Command Engine integration is not implemented.
- Status Bar and Message Bar are not implemented.
- Terminal resizing and frame reinitialization are not implemented.
- Incremental or dirty-cell rendering is not implemented.
- Automated tests do not yet comprehensively cover Renderer behavior, terminal integration, allocation-failure paths, or end-to-end interactions.

## Current Focus

**Chapter 48 — Working with Lines is implemented and verified.** The editor builds, all registered tests pass, and the user has confirmed that the editor works in the terminal.

## Next Step

**Chapter 49 — Navigating the Document**

Implement vertical Cursor movement, preferred-column behavior, Viewport state, vertical and horizontal scrolling, and Cursor visibility. Continue from the current source code and preserve established module boundaries. Identify every source file that must change, explain the reason for each change, and test navigation boundaries and viewport invariants. Update this document again after Chapter 49 implementation and verification.
