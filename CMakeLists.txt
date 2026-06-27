cmake_minimum_required(VERSION 3.20)
project(FuckMan VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

set(CORE_SOURCES
    src/core/orchestrator.c
    src/core/worker.c
    src/core/queue.c
    src/core/db_manager.c
    src/core/models.c
    src/core/logger.c
    src/core/assertion.c
    src/core/run_plan.c
    src/core/engine.c
)

add_library(httpruntime_core STATIC
    ${CORE_SOURCES}
)

target_compile_options(httpruntime_core PRIVATE -Wall -Wextra)

target_include_directories(httpruntime_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
)

target_link_libraries(httpruntime_core PUBLIC
    CURL::libcurl
    SQLite::SQLite3
    Threads::Threads
)

set(CLI_SOURCES
    src/cli/main.c
    src/cli/cli_output.c
    src/cli/cli_parser.c
    src/cli/commands.c
)

add_executable(fuckman
    ${CLI_SOURCES}
)

target_compile_options(fuckman PRIVATE -Wall -Wextra)

target_include_directories(fuckman PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/cli
)

target_link_libraries(fuckman PRIVATE
    httpruntime_core
)

enable_testing()
add_subdirectory(tests)