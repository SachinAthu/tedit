# Implementation Notes

## Volume X — Implementation Notes

Volume X is being implemented as a separate C99/Linux-POSIX implementation repository. The source code is the primary artifact, and implementation proceeds incrementally from the minimal runtime toward the complete terminal editor.

## Chapter 45 — Establishing the Runtime

Chapter 45 established the first executable runtime of the editor.

The implementation deliberately stops at the runtime boundary. Editor-specific components have not yet been introduced.

### Terminal State

A dedicated `terminal` module now owns terminal configuration.

The module maintains:

```text
TerminalState
├── original_attributes
├── initialized
└── raw_mode_enabled
```

The original terminal attributes are captured during initialization and retained until cleanup.

Raw mode is constructed from the saved attributes rather than modifying the saved copy directly.

The `raw_mode_enabled` state is set only after the terminal successfully accepts the new attributes.

### Raw Mode

The runtime now enables POSIX raw terminal behavior appropriate for the editor.

The implementation disables normal terminal processing required for interactive editor input, including:

- character echo,
- canonical input processing,
- signal generation,
- extended input processing,
- relevant input/output translations.

Input is configured so that the event loop can receive input without waiting for a complete terminal line.

### Terminal Cleanup

Terminal restoration is explicitly part of the application lifecycle.

The application does not simply terminate the process after entering raw mode. It first restores the original terminal attributes.

Cleanup is also safe when initialization succeeded but raw mode was never successfully enabled.

This establishes the runtime invariant:

```text
If raw mode was successfully enabled,
the application must restore the original terminal state
before normal termination.
```

### Application Lifecycle

The runtime now follows:

```text
Program Start
      ↓
Terminal Initialization
      ↓
Raw Mode Activation
      ↓
Event Loop
      ↓
Exit Condition
      ↓
Terminal Restoration
      ↓
Program Exit
```

Initialization failures prevent entry into the event loop.

If raw-mode activation fails after terminal initialization, cleanup is still performed before termination.

### Minimal Event Loop

The event loop is intentionally temporary and minimal.

Its current behavior is:

```text
Wait for one input byte
        │
        ├── Ctrl-Q → exit
        │
        ├── EOF → exit
        │
        └── other byte → ignore
```

The loop currently performs a blocking `read()` and reads one byte at a time.

The direct `Ctrl-Q` check is not intended to become the final input architecture. It will later be replaced by the established Input Handler → Input Parser → Input Engine → Command Engine pipeline.

### Source Files Added

Chapter 45 introduced:

```text
src/terminal/terminal.h
src/terminal/terminal.c
```

and modified:

```text
src/main.c
CMakeLists.txt
```

The terminal implementation is compiled into the `tedit` executable through CMake.

### Architectural Boundary

The terminal implementation is kept at the platform-facing boundary.

The current runtime does not introduce dependencies on:

```text
Document
Cursor
Viewport
Editor State
Renderer
Input Parser
Input Engine
Command Engine
File Manager
```

This preserves the staged construction defined for Volume X.

### Testing and Verification

The existing smoke test remains part of the CMake test configuration.

Chapter 45 also established manual runtime verification:

```text
Application starts
Raw mode is active
Characters are not normally echoed
Ctrl-Q exits
Terminal settings are restored
Shell input works normally afterward
```

The runtime milestone is considered complete only when the implementation and existing test baseline remain intact.

### Implementation Lesson

The first runtime milestone demonstrates an important implementation principle:

> Establish the application's lifecycle and ownership boundaries before adding editor behavior.

The terminal must be reliably initialized and restored before later components can safely depend on it.
