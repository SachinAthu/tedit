cmake_minimum_required(VERSION 3.20)
enable_testing()

project(
    terminal_editor
    VERSION 0.1.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(
    tedit
    src/main.c
    src/terminal/terminal.c
    src/state/editor_state.c
    src/document/document.c
    src/cursor/cursor.c
    src/renderer/screen_buffer.c
    src/renderer/renderer.c
)

target_include_directories(
    tedit
    PRIVATE
    src
)

add_executable(
    test_smoke
    tests/test_smoke.c
)

add_test(
    NAME smoke
    COMMAND test_smoke
)