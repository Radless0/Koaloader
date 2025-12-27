cmake_minimum_required(VERSION 3.24)

project(koaloader-test LANGUAGES CXX)

# List Directories test

add_executable(list_directory_test list_directory_test.cpp)
set_target_properties(list_directory_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# List Modules test

add_executable(list_modules_test list_modules_test.cpp)
set_target_properties(list_modules_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
