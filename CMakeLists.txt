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

option(
    TEDIT_ENABLE_DIAGNOSTICS
    "Enable development-time invariant diagnostics"
    OFF
)

option(
    TEDIT_ENABLE_LOGGING
    "Enable development logging to log.txt outside Release builds"
    ON
)

option(
    TEDIT_ENABLE_SANITIZERS
    "Enable AddressSanitizer and UndefinedBehaviorSanitizer"
    OFF
)

add_library(
    tedit_core
    STATIC
    src/common/logger.c
    src/diagnostics/diagnostics.c
    src/terminal/terminal.c
    src/input/input_parser.c
    src/input/input_engine.c
    src/command/command_engine.c
    src/state/editor_state.c
    src/document/document.c
    src/cursor/cursor.c
    src/text/text_layout.c
    src/viewport/viewport.c
    src/renderer/screen_buffer.c
    src/renderer/renderer.c
    src/file/file.c
    src/file/file_save.c
)

target_include_directories(tedit_core PUBLIC src)

target_compile_definitions(
    tedit_core
    PRIVATE
    "TEDIT_VERSION=\"${PROJECT_VERSION}\""
)

if(TEDIT_ENABLE_DIAGNOSTICS)
    target_compile_definitions(tedit_core PUBLIC TEDIT_ENABLE_DIAGNOSTICS=1)
endif()

# ADR-031: development builds carry observability; Release omits log calls.
# The logger implementation remains available to its isolated unit tests.
if(TEDIT_ENABLE_LOGGING)
    target_compile_definitions(
        tedit_core
        PUBLIC
        "$<$<NOT:$<CONFIG:Release>>:TEDIT_ENABLE_LOGGING=1>"
    )
endif()

if(TEDIT_ENABLE_SANITIZERS)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(
            tedit_core
            PUBLIC
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )
        target_link_options(
            tedit_core
            PUBLIC
            -fsanitize=address,undefined
        )
    else()
        message(FATAL_ERROR
            "TEDIT_ENABLE_SANITIZERS requires a GCC- or Clang-compatible compiler"
        )
    endif()
endif()

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
add_executable(test_input_parser tests/test_input_parser.c)
add_executable(test_input_engine tests/test_input_engine.c)
add_executable(test_command_engine tests/test_command_engine.c)
add_executable(test_input_pipeline tests/test_input_pipeline.c)
add_executable(test_terminal tests/test_terminal.c)
add_executable(test_logger tests/test_logger.c)

target_link_libraries(test_smoke PRIVATE tedit_core)
target_link_libraries(test_editing PRIVATE tedit_core)
target_link_libraries(test_file PRIVATE tedit_core)
target_link_libraries(test_file_save PRIVATE tedit_core)
target_link_libraries(test_input_parser PRIVATE tedit_core)
target_link_libraries(test_input_engine PRIVATE tedit_core)
target_link_libraries(test_command_engine PRIVATE tedit_core)
target_link_libraries(test_input_pipeline PRIVATE tedit_core)
target_link_libraries(test_terminal PRIVATE tedit_core)
target_link_libraries(test_logger PRIVATE tedit_core)

add_test(NAME smoke COMMAND test_smoke)
add_test(NAME editing COMMAND test_editing)
add_test(NAME file COMMAND test_file)
add_test(NAME file_save COMMAND test_file_save)
add_test(NAME input_parser COMMAND test_input_parser)
add_test(NAME input_engine COMMAND test_input_engine)
add_test(NAME command_engine COMMAND test_command_engine)
add_test(NAME input_pipeline COMMAND test_input_pipeline)
add_test(NAME terminal COMMAND test_terminal)
add_test(NAME logger COMMAND test_logger)

# Test assertions must remain active in Release configurations because tests
# use assert() to check results and, in some cases, invoke the operation under
# test. Production targets retain the selected build type's NDEBUG behavior.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    foreach(test_target IN ITEMS
        test_smoke
        test_editing
        test_file
        test_file_save
        test_input_parser
        test_input_engine
        test_command_engine
        test_input_pipeline
        test_terminal
        test_logger
    )
        target_compile_options(${test_target} PRIVATE -UNDEBUG)
    endforeach()
endif()

# End-to-end lifecycle checks use a POSIX pseudo-terminal when Python is available.
if(UNIX)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(
            NAME pty_integration
            COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_pty_integration.py
                    $<TARGET_FILE:tedit>
                    $<IF:$<AND:$<BOOL:${TEDIT_ENABLE_LOGGING}>,$<NOT:$<CONFIG:Release>>>,--expect-logging,--no-expect-logging>
        )
        set_tests_properties(pty_integration PROPERTIES TIMEOUT 20)
    endif()
endif()
