cmake_minimum_required(VERSION 3.20)
project(FuckMan VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

set(CORE_SOURCES 
    src/core/orchestrator.c
    src/core/worker.c
    src/core/queue.c
    src/core/db_manager.c
    src/core/models.c
    src/core/cli_output.c
    src/core/logger.c
    src/core/assertion.c
    src/core/cli_parser.c 
    src/core/commands.c
)

add_executable(fuckman
    src/core/main.c
    ${CORE_SOURCES}
)

target_compile_options(fuckman PRIVATE -Wall -Wextra)

target_include_directories(fuckman PRIVATE include/core)

target_link_libraries(fuckman PRIVATE 
    CURL::libcurl
    SQLite::SQLite3
    Threads::Threads
)

enable_testing()
add_subdirectory(tests)