# Implementation Status

## Project Status

**Active — Volume X implementation**

The runtime foundation and editing milestones through Chapter 48 are complete. Chapter 48 adds logical-line splitting and merging, including Enter and Backspace behavior at line boundaries. The user confirmed that the editor builds, works in the terminal, and all registered automated tests pass.

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
- delete the character before the Cursor using Backspace, or merge with the previous line when Backspace is pressed at column zero and a previous line exists;
- mark the Document dirty after successful insertion, deletion, line splitting, or line merging;
- redraw the screen after editing;
- exit using `Ctrl-Q`;
- clean up initialized resources and return control to the shell.

## Verification

### Build and automated tests

- [x] CMake configuration and build succeeded for the Chapter 48 implementation
- [x] `smoke` automated test passed
- [x] `editing` automated test passed, including line splitting and merging cases
- [x] User confirmed the editor builds and works in the terminal
- [ ] Comprehensive automated tests for Document, Cursor, Screen Buffer, and Renderer
- [ ] Integration and regression testing across failure paths
- [ ] Broader end-to-end verification during the hardening milestone

The current build and registered tests pass, and the user has confirmed interactive operation. Comprehensive correctness, allocation-failure coverage, and later end-to-end hardening remain outstanding.

## Implementation Status Summary

| Area                                                        | Status                                                                              |
| ----------------------------------------------------------- | ----------------------------------------------------------------------------------- |
| Repository and Git workflow                                 | Established                                                                         |
| CMake and C99 configuration                                 | Established                                                                         |
| Terminal initialization, raw mode, restoration, and cleanup | Implemented                                                                         |
| Application lifecycle and event loop                        | Implemented; input handling remains temporary                                       |
| Editor State                                                | Implemented                                                                         |
| Document                                                    | Vector of logical lines; insertion, deletion, splitting, and merging implemented    |
| Cursor                                                      | Validated position and horizontal movement within a line; vertical movement pending |
| Screen Buffer                                               | Implemented                                                                         |
| Renderer                                                    | Basic full-redraw implementation                                                    |
| Printable ASCII insertion                                   | Implemented                                                                         |
| Backspace character deletion and line merging               | Implemented                                                                         |
| Enter and logical-line splitting                            | Implemented                                                                         |
| Dirty Flag                                                  | Updated after successful insertion, deletion, splitting, and merging                |
| Full document navigation                                    | Not implemented; Chapter 49 next                                                    |
| Viewport and scrolling                                      | Not implemented; Chapter 49 next                                                    |
| File loading                                                | Not implemented                                                                     |
| File saving                                                 | Not implemented                                                                     |
| Dedicated Input Parser and command pipeline                 | Not implemented; basic input recognition remains in the event loop                  |
| Status Bar and Message Bar                                  | Not implemented                                                                     |
| Terminal resize handling                                    | Not implemented                                                                     |
| Automated tests                                             | `smoke` and `editing` pass; comprehensive coverage pending                          |
| Hardening and final integration                             | Pending                                                                             |

## Current Milestone

**Chapter 48 — Working with Lines: implemented and verified**

Chapter 48 adds Document line splitting and merging. Enter and line feed split the current line at the Cursor; Backspace at column zero merges with the previous line when possible. Cursor placement, Dirty Flag updates, and redraws are coordinated in the existing event loop. The user confirmed that the editor builds and works in the terminal and all registered tests pass.

## Next Milestone

**Chapter 49 — Navigating the Document**

Planned focus, following the established implementation sequence:

- implement vertical Cursor movement across logical lines;
- define and implement preferred-column behavior for Up and Down movement;
- introduce or complete Viewport state and Cursor visibility handling;
- implement vertical and horizontal scrolling as specified by the project architecture and ADRs;
- test navigation boundaries, short and empty lines, and viewport invariants.

Continue from the current source code. Preserve established module boundaries and identify every changed file before implementing the milestone.

## Current Limitations

- Only printable ASCII characters are accepted for insertion.
- The Delete key is not implemented.
- Cursor movement is limited to Left and Right within a single line; vertical movement and full document navigation are not implemented.
- Viewport-based scrolling is not implemented, and insertion is constrained by the current terminal-width behavior.
- Input handling remains temporarily inside the application event loop rather than a dedicated Input Parser and command pipeline.
- File opening and saving are not implemented.
- The Status Bar, Message Bar, and terminal resize handling are not implemented.
- Comprehensive automated testing, failure-path regression testing, hardening, and final integration remain future work.
