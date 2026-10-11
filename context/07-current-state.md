# Current State

## Project

Terminal text editor implementation in C99 for Linux/POSIX, developed incrementally using CMake.

## Current Milestone

**Chapter 47 — Editing Text**

Chapter 47 extends the first editable screen with character deletion, horizontal Cursor movement, Backspace handling, and the Dirty Flag. The implementation continues to use the existing Document, Cursor, Editor State, Renderer, and Terminal modules.

The project was configured and built successfully from the Chapter 47 repository snapshot. Both registered CTest tests (`smoke` and `editing`) passed in that verification run. Interactive terminal behavior still requires manual verification in the target WSL/Linux terminal.

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
- Cursor advancement supports the initial character-insertion workflow.
- Cursor operations are implemented in the `cursor` module.

#### Editor State

- `EditorState` owns the Document and Cursor.
- Editor State initialization establishes the initial editing state.
- Editor State destruction releases its owned Document resources.
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

- `document_delete_character()` removes a character at a valid row and column.
- Deletion shifts the following bytes left and decreases the line length.
- Invalid rows and columns are rejected without modifying the line.
- The Document remains a vector of logical lines; line splitting and merging are not implemented in this milestone.

#### Cursor Movement

- `cursor_move_left()` moves left within the current logical line and refuses to move before column zero.
- `cursor_move_right()` moves right within the current line and refuses to move beyond the line length.
- Movement across logical line boundaries is not implemented.

#### Dirty Flag

- `EditorState` contains the `is_dirty` Boolean field.
- The flag initializes to `false`.
- Successful character insertion sets the flag to `true`.
- Successful Backspace deletion sets the flag to `true`.
- Failed or out-of-bounds deletion does not mark the Document dirty.
- There is no file-saving workflow yet, so the flag is not cleared after saving.

#### Interactive Editing Loop

- `Ctrl-Q` exits the editor.
- EOF terminates the event loop.
- Printable ASCII characters are inserted at the current Cursor position.
- The Cursor advances after successful insertion.
- Backspace is recognized as byte `0x7F` or `0x08`; it deletes the character before the Cursor when the Cursor column is greater than zero.
- Left and Right Arrow input is recognized using the current basic `ESC [` sequence handling.
- Successful movement and text edits trigger a full redraw.
- Unsupported control characters and non-ASCII input bytes are ignored.
- The Delete key is **not** implemented; its terminal escape sequence is not dispatched to a delete-at-cursor operation.
- Insertion remains restricted near the right edge because Viewport scrolling has not yet been implemented.
- Input handling remains temporarily inside `main.c`; it is not yet the final Input Handler, Input Parser, Input Engine, or Command Engine architecture.

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
      +---- Backspace ------------------> Delete Before Cursor
      |                                      |
      |                                      v
      |                                 Move Cursor Left
      |                                      |
      |                                      v
      |                                 Set Dirty Flag
      |                                      |
      |                                      v
      |                                 Render Frame
      |
      +---- Left / Right Arrow ---------> Move Cursor
                                             |
                                             v
                                        Render if moved
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
│   └── chapter-47.md
├── context/
├── CMakeLists.txt
└── README.md
```

## Verification Status

### Build and Automated Tests

Verified from the Chapter 47 repository snapshot:

- CMake configuration completed successfully.
- The `tedit`, `test_smoke`, and `test_editing` targets built successfully.
- CTest passed both registered tests: `smoke` and `editing`.
- The editing test covers Document insertion/deletion, invalid deletion positions, horizontal Cursor movement boundaries, and initial Dirty Flag state.

### Manual Verification

The following Chapter 46 behavior was previously reported as manually verified:

- Initial editor screen displayed correctly.
- Printable ASCII insertion and Cursor advancement worked.
- Repeated typing updated the displayed document.
- The terminal cursor rendering glitch was addressed by hiding the terminal cursor during rendering.
- `Ctrl-Q` exit, terminal restoration, and post-exit shell behavior were checked.

The following Chapter 47 behavior still requires manual verification in the target terminal:

- Backspace removes the character before the Cursor.
- Left and Right Arrow keys move the Cursor within a line.
- Editing and movement redraw the screen correctly.
- `Ctrl-Q` still exits cleanly and restores the terminal.

Automated tests do not establish that interactive key sequences behave identically across all terminals.

## Architectural Boundaries

- **Terminal:** terminal configuration, raw mode, restoration, and alternate-screen lifecycle.
- **Document:** logical lines and character storage, including insertion and deletion.
- **Cursor:** logical editing position and horizontal movement within a line.
- **Editor State:** ownership and coordination of the Document and Cursor, including the Dirty Flag.
- **Screen Buffer:** in-memory representation of the visible frame.
- **Renderer:** frame construction and terminal output; it remains a read-only consumer of Editor State.
- **Application Event Loop:** temporary direct input handling and coordination of editing, Cursor movement, Dirty Flag updates, and rendering.

The Screen Buffer is presentation data and does not replace the Document as the authoritative text representation.

## Known Limitations

- Delete-key behavior is not implemented.
- Enter and logical-line splitting are not implemented.
- Backspace at column zero does not merge with the previous line.
- Cursor movement across logical line boundaries is not implemented.
- Vertical navigation and preferred-column handling are not implemented.
- Viewport management and horizontal/vertical scrolling are not implemented.
- Editing remains restricted near the right edge of the visible screen.
- Interactive input supports printable ASCII only; UTF-8-aware input is not implemented.
- File loading and saving are not implemented.
- The Dirty Flag is not yet connected to save behavior.
- Input Handler, Input Parser, Input Engine, and Command Engine integration is not implemented.
- Status Bar and Message Bar are not implemented.
- Terminal resizing and frame reinitialization are not implemented.
- Incremental or dirty-cell rendering is not implemented.
- Automated tests do not yet comprehensively cover Renderer behavior, terminal integration, failure paths, or interactive editing.

## Current Focus

Chapter 47 — Editing Text is implemented in the current repository snapshot. The project builds, and the smoke and editing tests pass. Manual verification of Chapter 47 keyboard behavior remains.

## Next Step

**Chapter 48 — Working with Lines**

Continue from the current source state to implement line splitting, line merging, end-of-line behavior, Cursor transitions across logical lines, and editing invariants. Preserve the existing Document, Cursor, Editor State, Renderer, and Terminal boundaries. Update this document again if implementation or verification reveals a discrepancy.
