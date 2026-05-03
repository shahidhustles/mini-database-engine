cmake_minimum_required(VERSION 3.16)
project(mini_db_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

set(COMMON_SOURCES
    src/huffman.cpp
    src/red_black_tree.cpp
    src/skip_list.cpp
    src/memory_layer.cpp
    src/bplus_tree.cpp
    src/storage_layer.cpp
    src/db_interface.cpp
    src/command_parser.cpp
)

add_executable(mini_db_engine
    src/main.cpp
    ${COMMON_SOURCES}
)

add_executable(mini_db_engine_tests
    tests/test_main.cpp
    ${COMMON_SOURCES}
)
