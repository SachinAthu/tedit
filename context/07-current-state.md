# Current State

## Project

Terminal text editor implementation in C99 for Linux/POSIX, developed incrementally using CMake. The implementation repository is separate from the book repository and follows the architecture, software requirements, runtime model, coding standards, and ADRs recorded in `context/`.

## Current Milestone

**Chapter 52.8 — Interface Refinement and Unsaved-Changes Confirmation implemented**

The interface layout now places the Status Bar on the bottom row, reserves one blank separator row above the white Message Bar, and renders Message Bar text in black. During filename entry, the prompt and terminal cursor use the Message Bar row. Filename editing supports Left/Right, insertion at the cursor, Backspace, and Delete; movement and deletion respect UTF-8 byte boundaries.

Ctrl-Q exits immediately for a clean Document. For a dirty Document, it prompts for a decision: `Y` saves and quits, `N` discards changes and quits, and Escape cancels quitting. Saving an unnamed Document opens filename entry; failed saves keep the editor open and preserve the Dirty Flag. No separate Save As command for named documents has been added: Ctrl-S retains save-to-current-path behavior.

Debug and warning-clean builds succeed, and all eight CTest targets pass in both configurations. Pseudo-terminal integration checks passed for filename editing and saving, Message Bar styling, and dirty-quit confirmation/cancellation/discard.

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
- `editor_state_save_file()` clears the flag only after a successful File Manager save; interactive save integration is pending.

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
- **File Manager:** complete-file POSIX reads, strict UTF-8 decoding, line-ending normalization, UTF-8 serialization, and temporary-file replacement.
- **Application Event Loop:** temporary direct input handling and coordination of editing, Cursor movement, Dirty Flag updates, and rendering.

The Screen Buffer is presentation data and does not replace the Document as the authoritative text representation.

## Known Limitations

- Non-ASCII filename bytes are preserved for filesystem operations but currently displayed as `?` in the Status Bar and filename-entry prompt.
- Delete is supported only during filename entry. Document Delete behavior remains a navigation/editing follow-up, as agreed.
- Home, End, Page Up, Page Down, and Tab support remain deferred to the earlier navigation and editing milestones.
- No separate Save As command exists for an already-associated document; Ctrl-S saves to its current path.
- Terminal resizing and Screen Buffer reinitialization are not implemented.
- Incremental or dirty-cell rendering is not implemented.
- Allocation-failure paths and a broader terminal-size integration matrix need additional coverage.

## Current Focus

**Chapter 52 interface refinement is complete.** Status Bar/Message Bar placement, filename cursor editing, and unsaved-quit confirmation are implemented and regression-tested.

## Next Step

**Chapter 53 — Hardening.** Begin invariant checks, boundary/failure-path review, sanitizer builds, and broader regression coverage. Navigation additions and Tab support remain tracked as follow-up work for their earlier implementation milestones.

## Chapter 51 Progress — Saving Through Editor State (Sections 51.2–51.3)

The File Manager serializes a complete Document as UTF-8 and writes canonical LF after every logical line. A zero-line Document serializes to a zero-byte file. Saving uses a temporary file created beside the destination, checks buffered writes (including partial writes and `EINTR`), closes the file, and renames it over the destination only after successful serialization. Failed saves remove the temporary file where possible and do not replace the destination.

`editor_state_save_file()` coordinates saving for Editor State. It returns success only when the File Manager returns `FILE_SAVE_OK`; only then does it clear `is_dirty`. On invalid arguments or any File Manager failure, the Dirty Flag and in-memory Document remain unchanged. Ctrl-S invokes this API when the application has a startup filename. Ctrl-S remains a no-op for unnamed Documents; visible feedback is deferred until the Message Bar exists.

Verification for this snapshot: CMake build succeeded and all four registered CTest targets (`smoke`, `editing`, `file`, and `file_save`) passed. The save tests cover UTF-8 output, LF after each logical line, zero-line output, successful Dirty Flag clearing, and failure preservation of the Document, Dirty Flag, and existing destination contents.


## Chapter 51 Progress — Event Loop Integration (Section 51.4)

The existing direct input loop now recognizes Ctrl-S (`0x13`). When the application was started with a filename, Ctrl-S calls `editor_state_save_file()` for that path. A failed save does not clear the Dirty Flag and does not terminate the Event Loop. Ctrl-S on an unnamed Document is currently a no-op; Save As is not implemented. Visible save feedback is deferred until the Message Bar exists, rather than adding a second terminal-output path in `main.c`.

Verification: CMake configure/build succeeded; all four CTest targets passed. A pseudo-terminal integration check launched the editor with a file, inserted a character, sent Ctrl-S and Ctrl-Q, and confirmed the destination contained the updated text.


### Chapter 51 Progress — Failure-Path Review (Section 51.5)

Added a replacement-failure regression test by attempting to save over an existing directory. The test confirms that the operation fails, the Editor State Dirty Flag remains set, the in-memory Document remains unchanged, the directory remains intact, and the temporary file is removed. This complements the existing invalid-code-point failure test, which verifies that an existing destination file is not replaced.

Verification: CMake configure/build succeeded and all four CTest targets (`smoke`, `editing`, `file`, and `file_save`) passed. Save success was already checked through a pseudo-terminal session in Section 51.4; visible success/failure feedback remains deferred until the Message Bar is available.

## Current Implementation Update — Chapter 52.5

The Input Engine and Command Engine are now implemented as separate modules. The Input Engine maps parser events to command values. The Command Engine executes insert, Backspace, newline, cursor movement, save, and quit commands, and reports redraw/quit decisions to the Event Loop. `main.c` retains the POSIX Input Handler and coordinates the full pipeline. All seven CTest targets pass in both Debug and warning-clean builds; a pseudo-terminal integration check passed for editing, Ctrl-S, and Ctrl-Q. Save As and filename-entry remain unimplemented. Next: Chapter 52.6 integration review and key-binding boundary.


## Chapter 52.7 — Filename Entry and Save As

Ctrl-S saves to the current associated path. When no path is associated, it activates a bounded filename-entry mode. Printable input is encoded as UTF-8, Backspace removes the preceding code point, Enter attempts the safe save, and Escape cancels after the Input Handler timeout. The Renderer displays the `Save as:` prompt in the Status Bar and places the terminal cursor at the entry point.

`editor_state_save_as()` allocates the candidate path before writing and updates `file_path` plus the Dirty Flag only after successful save. Failure keeps the prompt active for correction or cancellation; cancellation and failure preserve the previous association, Document, and Dirty Flag.

All eight CTest targets pass in Debug and warning-clean builds. Pseudo-terminal checks verified Save As persistence and Escape cancellation.


## Chapter 52.8 — Interface Refinement and Unsaved-Changes Confirmation

The Status Bar is now the final terminal row. The row immediately above it is the Message Bar, rendered with ANSI black foreground and white background; a blank row separates the Message Bar from the document area. Filename entry is rendered in the Message Bar, with cursor placement based on the UTF-8 byte boundary tracked by Editor State. Left/Right move between code-point boundaries, insertion occurs at the cursor, Backspace removes the preceding code point, and the Delete escape sequence (`CSI 3 ~`) removes the code point at the cursor. Document Delete remains intentionally unimplemented.

Ctrl-Q exits immediately only when the Document is clean. When dirty, the Message Bar asks whether to save: `Y` saves and quits, `N` quits without saving, and Escape cancels. If the Document is unnamed, choosing `Y` starts filename entry and the application exits only after a successful save. Save failure preserves the Dirty Flag and keeps the editor active. No separate Save As command for named documents was added; Ctrl-S retains the current behavior.

Verification: all eight CTest targets passed in Debug and warning-clean builds (`-Wall -Wextra -Wpedantic -Werror`). Pseudo-terminal checks verified filename insertion/movement/Delete/save, the Message Bar ANSI style, and dirty-quit confirmation, cancellation, and discard.
