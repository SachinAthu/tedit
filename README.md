# Terminal Text Editor

A small, professional-quality terminal text editor written in **C99** for **Linux/POSIX** systems.

The project focuses on building a real, maintainable terminal editor from the ground up, with an emphasis on clear separation of responsibilities, incremental development, testing, and understanding how a text editor works internally.

> **Status:** Active development

## Features

The editor is being developed around the following capabilities:

- Terminal-based text editing
- File opening and saving
- Cursor movement and text editing
- Vertical and horizontal scrolling
- UTF-8 text handling
- Status and message bars
- Configurable editor settings
- Configurable key bindings
- Automated testing
- Debugging and development support

Some features may still be under development.

## Technology

- **Language:** C99
- **Platform:** Linux/POSIX
- **Build system:** CMake
- **Terminal:** POSIX terminal facilities and ANSI-compatible terminal output
- **Version control:** Git

## Building

### Requirements

A development environment with:

- A C99-compatible compiler
- CMake
- Make or another supported build backend
- A POSIX-compatible terminal environment

### Build

Clone the repository:

```bash
git clone <repository-url>
cd <repository-directory>
```

Configure the build:

```bash
cmake -S . -B build
```

Build the editor:

```bash
cmake --build build
```

The exact executable location may depend on the CMake configuration.

## Running

After building, start a new document with:

```bash
./build/tedit
```

To open an existing file:

```bash
./build/tedit path/to/file.txt
```

The application currently accepts no filename or one filename. File loading supports UTF-8 and normalizes LF and CRLF line endings. File saving is planned for Chapter 51.

## Testing

Tests are part of the project and should be run regularly during development.

Build the test targets:

```bash
cmake --build build
```

Run the test suite:

```bash
ctest --test-dir build --output-on-failure
```

Additional tests may be added as the editor grows.

## Development

Development follows an incremental approach:

```text
Understand
    ↓
Design
    ↓
Implement
    ↓
Compile
    ↓
Test
    ↓
Inspect
    ↓
Refactor
    ↓
Commit
```

Changes should generally be small and focused. New functionality should be tested where practical, and bugs should be reproduced and understood before being fixed.

The project is intended to evolve from a working core editor into a more capable and robust application over time.

## Documentation

Additional technical documentation is maintained separately from this README.

The README provides an overview of the project and basic usage. Detailed requirements, implementation decisions, architecture documentation, and development conventions are maintained in their respective project documents.

## Project Goals

The project aims to provide:

- A practical terminal text editor
- A clean and maintainable C codebase
- A clear understanding of the components involved in a text editor
- Testable and incremental development
- A foundation that can be extended with more advanced editor functionality

The emphasis is on **building a working system carefully**, rather than maximizing the number of features as quickly as possible.

## Contributing

Contributions, suggestions, bug reports, and discussions are welcome.

When contributing code:

1. Keep changes focused.
2. Follow the existing project structure.
3. Test changes before submitting them.
4. Avoid unnecessary rewrites.
5. Document significant behavior or implementation changes where appropriate.

## License

This project is free and open source software, released under the [MIT License](LICENSE).

You are free to use, copy, modify, merge, publish, distribute, sublicense, and sell copies of the software, provided the copyright notice and permission notice are included. The software is provided "as is", without warranty of any kind.

---

**Terminal Text Editor** — built from the ground up in C.

## Diagnostic Logging

Development builds write diagnostic records to `log.txt` in the process's current working directory. Each run starts a fresh log, replacing the previous session's records. The first record names the build version and process ID, and every record includes a local timestamp, severity, component, and message.

The logger supports `ERROR`, `WARN`, `INFO`, and `DEBUG`. Debug builds include all levels; Release builds compile out application logging by default. Logging can be disabled explicitly with `-DTEDIT_ENABLE_LOGGING=OFF`. Failure to open or write the log is non-fatal and does not change Editor State or the editing pipeline. Document contents are never logged.

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug -DTEDIT_ENABLE_LOGGING=ON
cmake --build build/debug
ctest --test-dir build/debug --output-on-failure
```
