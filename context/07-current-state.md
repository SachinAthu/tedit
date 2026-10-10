# Current State

## Project

Terminal text editor implementation in C99 for Linux/POSIX, developed incrementally using CMake.

## Current Milestone

**Chapter 46 — Building the First Editable Screen**

Chapter 46 has been implemented. The application now initializes its core editing state, renders an empty document, accepts printable ASCII characters, inserts them into the Document, advances the Cursor, and redraws the screen.

The project builds successfully, and the manual verifications performed for the current implementation have passed.

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
- Cleanup occurs before normal application termination.
- Terminal management is isolated in the `terminal` module.
- CMake updated to compile the terminal implementation.

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

- Cursor stores a logical row and column.
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
- Screen Buffer storage is bounded by the terminal dimensions.

#### Renderer

- Renderer constructs a complete frame from the current Editor State.
- Document characters are copied into the visible Screen Buffer.
- Content beyond the current screen dimensions is clipped.
- Terminal output uses ANSI cursor-positioning sequences.
- The terminal text cursor is hidden during frame rendering and restored afterward.
- Frame output is assembled in a reusable, dynamically allocated output buffer.
- The output buffer grows when necessary and is reused across subsequent frames.
- The output helper handles partial writes and retries interrupted writes.
- Rendering remains based on the full-redraw strategy established by the project architecture.

#### Interactive Editing Loop

- The event loop receives input one byte at a time.
- `Ctrl-Q` exits the editor.
- EOF terminates the event loop.
- Printable ASCII characters are accepted for insertion.
- Character insertion updates the Document at the current Cursor position.
- The Cursor advances after successful insertion.
- The Renderer redraws the frame after each accepted character.
- Unsupported control characters and non-ASCII bytes are ignored.
- Insertion is restricted near the right edge because viewport scrolling has not yet been implemented.
- Initialization and runtime failures follow the application cleanup path.

#### Rendering Performance

- Individual screen-cell writes have been replaced by batched frame output.
- ANSI row-positioning sequences and screen characters are assembled before writing.
- The output buffer is reused between frames rather than allocated anew for every redraw.
- The Renderer still performs a full redraw for each accepted character.
- Dirty-cell tracking and incremental rendering have not been implemented.

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
      +---- Ctrl-Q / EOF ------> Normal Exit
      |
      +---- Printable ASCII --> Insert Character
                                      |
                                      v
                                 Advance Cursor
                                      |
                                      v
                                 Render Frame
                                      |
                                      +----> Read Next Input
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
Program Exit
```

If initialization fails, the application skips the event loop and enters cleanup for resources that were successfully initialized.

If character insertion or rendering fails, the event loop reports failure to the application, which proceeds through the cleanup path.

## Current Source Structure

The relevant implementation modules are:

```text
src/
├── main.c
├── terminal/
│   ├── terminal.c
│   └── terminal.h
├── document/
│   ├── document.c
│   └── document.h
├── cursor/
│   ├── cursor.c
│   └── cursor.h
├── state/
│   ├── editor_state.c
│   └── editor_state.h
└── renderer/
    ├── renderer.c
    ├── renderer.h
    ├── screen_buffer.c
    └── screen_buffer.h
```

The existing CMake configuration and smoke-test configuration remain part of the project.

## Verification Status

### Build

- CMake configuration and project build completed successfully.

### Manual Verification

- Initial editor screen displayed correctly.
- Printable ASCII character insertion worked.
- Cursor advancement worked.
- Repeated typing updated the displayed document.
- The rendering-related cursor flicker observed in Windows Terminal under WSL was resolved by hiding the terminal text cursor during frame rendering.
- `Ctrl-Q` exit behavior worked.
- Terminal restoration and post-exit shell behavior were verified.
- EOF behavior was considered separately from interactive `Ctrl-D` input because the application uses raw terminal mode.

These results reflect the reported local build and manual verification. They do not imply that every future editing operation or unimplemented feature has been tested.

## Architectural Boundaries

The implementation preserves the following responsibilities:

- **Terminal:** terminal configuration, raw mode, and restoration.
- **Document:** logical lines and character storage.
- **Cursor:** logical editing position.
- **Editor State:** ownership and coordination of the current editing state.
- **Screen Buffer:** in-memory representation of the visible frame.
- **Renderer:** frame construction and terminal output.
- **Application Event Loop:** temporary direct input handling and coordination of insertion, cursor advancement, and rendering.

The direct input handling in `main.c` is temporary. It is not the final Input Handler, Input Parser, Input Engine, or Command Engine architecture.

The Renderer remains a read-only consumer of Editor State. The Screen Buffer contains presentation data and does not replace the Document as the authoritative text representation.

## Known Limitations

The current implementation is the first editable-screen milestone, not a complete text editor.

The following capabilities remain unimplemented:

- Character deletion and Backspace.
- Enter and logical-line splitting.
- Multiple-line editing.
- Full cursor navigation.
- Preferred-column handling for vertical movement.
- Viewport management and scrolling.
- Editing beyond the current visible boundaries.
- UTF-8 character decoding for interactive input.
- File loading and saving.
- Dirty Flag management.
- Input Parser, Input Engine, and Command Engine integration.
- Status Bar and Message Bar.
- Terminal resizing and frame reinitialization.
- Incremental or dirty-cell rendering.
- Comprehensive automated tests for Document, Cursor, Screen Buffer, Renderer, and editing invariants.

The current insertion-boundary restriction is a temporary consequence of not yet having viewport scrolling. It must not be mistaken for the final editing behavior.

## Current Focus

Chapter 46 is implemented and its current build and manual verification milestone is complete.

The next implementation chapter introduces the editing operations required to modify existing text rather than only insert characters into an initially empty line.

## Next Step

**Chapter 47 — Editing Text**

The next milestone will build on the existing Document, Cursor, Editor State, and Renderer to implement character deletion, cursor movement, basic editing commands, and Dirty Flag behavior, following the established project architecture.
