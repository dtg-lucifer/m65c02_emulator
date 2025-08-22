cmake_minimum_required(VERSION 3.10)
project(m6502)

# Find thread library
find_package(Threads REQUIRED)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enable debug symbols and warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")

# Set global include directories (will be inherited by all targets)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Generate compile_commands.json for tooling (helps IDEs find symbols)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Create a library for the core emulator code
add_library(emulator_core
    lib/wdc65c02.cpp
    lib/at28c256.cpp
    lib/hm62256b.cpp
    lib/mm_clock.cpp
    lib/bus.cpp
    lib/decoder.cpp
)

# Link against thread library
target_link_libraries(emulator_core PUBLIC Threads::Threads)

# Make includes available to any target linking against emulator_core
target_include_directories(emulator_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(m6502
    src/main.cpp
)

# Link the main executable with the core library
target_link_libraries(m6502 PRIVATE emulator_core)

# Create a symbolic link to compile_commands.json in the source directory
# This helps many IDEs find the compilation database
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    )
endif()
