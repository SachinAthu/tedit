# Implementation Status

## Project Status

**Active — Volume X implementation**

Chapters 45–49 are implemented for the current ASCII-only scope. Chapter 49 adds vertical Cursor movement, preferred-column behavior, Viewport state, and vertical and horizontal scrolling. The user confirmed that the project builds, registered tests pass, vertical scrolling works, and the horizontal-edge crash has been fixed in WSL.

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
| Application lifecycle and event loop                        | Implemented; input handling remains temporary                                                       |
| Editor State                                                | Implemented                                                                                         |
| Document                                                    | Vector of logical lines; insertion, deletion, splitting, and merging implemented                    |
| Cursor                                                      | Validated logical position, horizontal and vertical movement, preferred-column behavior implemented |
| Viewport                                                    | Vertical and horizontal offsets and Cursor visibility maintenance implemented                       |
| Screen Buffer                                               | Implemented                                                                                         |
| Renderer                                                    | Full redraw of the visible document region; reserves the final physical terminal column             |
| Printable ASCII insertion                                   | Implemented                                                                                         |
| Backspace character deletion and line merging               | Implemented                                                                                         |
| Enter and logical-line splitting                            | Implemented                                                                                         |
| Dirty Flag                                                  | Updated after successful insertion, deletion, splitting, and merging                                |
| Full document navigation                                    | Implemented for the current ASCII-only model                                                        |
| Viewport and scrolling                                      | Implemented; vertical and horizontal behavior manually verified in WSL                              |
| File loading                                                | Not implemented                                                                                     |
| File saving                                                 | Not implemented                                                                                     |
| Dedicated Input Parser and command pipeline                 | Not implemented; basic input recognition remains in the event loop                                  |
| Status Bar and Message Bar                                  | Not implemented                                                                                     |
| Terminal resize handling                                    | Not implemented                                                                                     |
| Unicode-aware input and display-width mapping               | Not implemented                                                                                     |
| Automated tests                                             | `smoke` and `editing` pass; broader coverage pending                                                |
| Hardening and final integration                             | Pending                                                                                             |

## Current Milestone

**Chapter 49 — Navigating the Document: implemented and verified for the current ASCII-only scope**

Chapter 49 adds vertical Cursor movement with preferred-column behavior, Viewport state, and vertical and horizontal scrolling. The Renderer draws the visible document region using Viewport offsets. A WSL-specific right-edge failure was addressed by initializing the Renderer with one fewer column than the terminal reports, leaving the final physical column unused. The user confirmed that vertical scrolling and the horizontal typing crash are resolved. The current build and registered tests pass.

## Next Milestone

**Chapter 50 — Opening Files**

Continue from the current source code. Preserve established module boundaries and identify every changed file before implementing the milestone. Implement file loading through the planned File Manager boundary; do not move file-system responsibilities into the Document, Cursor, or Renderer.

## Current Limitations

- Only printable ASCII characters are accepted for insertion.
- The terminal Delete key is not implemented; Backspace is supported.
- UTF-8-aware input and Unicode display-width mapping are not implemented.
- Tab expansion is not implemented.
- File opening and saving are not implemented.
- The Dirty Flag is not yet connected to a save workflow.
- Input handling remains temporarily inside the application event loop rather than a dedicated Input Parser, Input Engine, and Command Engine pipeline.
- The Status Bar and Message Bar are not implemented.
- Terminal resize handling and frame reinitialization are not implemented.
- Comprehensive automated testing, broader failure-path regression testing, hardening, and final integration remain future work.
- The Renderer deliberately reserves the terminal's final physical column to avoid the right-edge wrapping behavior observed in WSL; this reduces the usable rendering width by one column.
