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
)

add_executable(
    test_smoke
    tests/test_smoke.c
)

add_test(
    NAME smoke
    COMMAND test_smoke
)