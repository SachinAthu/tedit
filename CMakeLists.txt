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

add_library(
    tedit_core
    STATIC
    src/terminal/terminal.c
    src/state/editor_state.c
    src/document/document.c
    src/cursor/cursor.c
    src/viewport/viewport.c
    src/renderer/screen_buffer.c
    src/renderer/renderer.c
    src/file/file.c
    src/file/file_save.c
)

target_include_directories(tedit_core PUBLIC src)

add_executable(
    tedit
    src/main.c
)

target_link_libraries(tedit PRIVATE tedit_core)

# tests

add_executable(test_smoke tests/test_smoke.c)
add_executable(test_editing tests/test_editing.c)
add_executable(test_file tests/test_file.c)
add_executable(test_file_save tests/test_file_save.c)

target_link_libraries(test_smoke PRIVATE tedit_core)
target_link_libraries(test_editing PRIVATE tedit_core)
target_link_libraries(test_file PRIVATE tedit_core)
target_link_libraries(test_file_save PRIVATE tedit_core)

add_test(NAME smoke COMMAND test_smoke)
add_test(NAME editing COMMAND test_editing)
add_test(NAME file COMMAND test_file)
add_test(NAME file_save COMMAND test_file_save)