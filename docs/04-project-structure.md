# Project Structure

## Directory Layout

The project follows a module-oriented directory structure.

Each module owns its own directory containing both its public interface (`.h`) and implementation (`.c`).

```text
editor/
│
├── command/
│   ├── command.h
│   └── command.c
│
├── cursor/
│   ├── cursor.h
│   └── cursor.c
│
├── document/
│   ├── document.h
│   └── document.c
│
├── file/
│   ├── file.h
│   └── file.c
│
├── input/
│   ├── input.h
│   └── input.c
│
├── renderer/
│   ├── renderer.h
│   └── renderer.c
│
├── state/
│   ├── state.h
│   └── state.c
│
├── terminal/
│   ├── terminal.h
│   └── terminal.c
│
├── text/
│   ├── text_layout.h
│   └── text_layout.c
│
├── viewport/
│   ├── viewport.h
│   └── viewport.c
│
├── common/
│   |── common.h
│   └── common.c
│
├── tests/
├── docs/
├── main.c
├── CMakeLists.txt
└── README.md
```

## Module Rules

- One module per directory.
- Each module owns its own `.h` and `.c` files.
- Public interfaces belong in the module header.
- Module internals remain private to the implementation file.
- `main.c` coordinates the application but contains minimal logic.
- The `input/` module houses all three input stages—Input Handler, Input Parser, and Input Engine—as separate functions within one module.

## Dependencies

Allowed dependency direction:

```text
Application
    │
    ▼
Core
    ▲
    │
Platform
```

Rules:

- Dependencies must be one-way.
- Circular dependencies are prohibited.
- Modules depend only on the interfaces they require.
- Renderer never modifies editor state.
- Document never depends on Renderer.
- Cursor and Renderer share display-width calculations through `text/text_layout`; Cursor never depends on Renderer.
- File Manager never depends on Renderer.
- Input never depends on File Manager.

## Common Module

`common/common.h` contains only project-wide shared definitions.

Allowed contents:

- Shared types
- Shared enums
- Shared constants
- Utility declarations
- Common includes

It must not contain module-specific functionality.

## Build System

- Build system: **CMake**
- Root build file: `CMakeLists.txt`
