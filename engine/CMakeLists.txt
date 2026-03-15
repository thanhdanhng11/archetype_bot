cmake_minimum_required(VERSION 3.14)
project(ArchetypeEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Map the include and library paths specifically for macOS (Apple Silicon & Intel)
include_directories(/opt/homebrew/include /usr/local/include)
link_directories(/opt/homebrew/lib /usr/local/lib)

# Define the final executable binary
add_executable(archetype_engine 
    src/main.cpp
    src/ipc_receiver.cpp
    src/tx_builder.cpp
    src/rpc_client.cpp
)

# Link the required high-speed C libraries
target_link_libraries(archetype_engine hiredis curl)