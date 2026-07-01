cmake_minimum_required(VERSION 3.20)
project(expense-bot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure clang-tidy integration if available
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-extra-arg=-Wno-unknown-warning-option")
else()
    message(WARNING "clang-tidy not found.")
endif()

# Find dependencies via vcpkg toolchain
find_package(dpp REQUIRED)
find_package(libpqxx REQUIRED)

# Include directories
include_directories(include)
include_directories(include/models)

# Define source files
set(SOURCES
    src/main.cpp
    src/connection_pool.cpp
    src/worker_pool.cpp
    src/db_processor.cpp
    src/gemini_client.cpp
    src/processor.cpp
    src/replier.cpp
    src/receiver.cpp
)

# Define your bot executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE dpp::dpp libpqxx::pqxx)

# Copy executable to a predictable location for simplified Docker COPY
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
