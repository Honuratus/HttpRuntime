# unity.c dosyası src klasörünün içinde
add_library(unity ../third_party/unity/src/unity.c)
target_include_directories(unity PUBLIC ../third_party/unity/src)

# 1. src/core içindeki TÜM .c dosyalarını topla
file(GLOB CORE_SOURCES "${CMAKE_SOURCE_DIR}/src/core/*.c")
file(GLOB DSL_SOURCES "${CMAKE_SOURCE_DIR}/src/dsl/*.c")


# 2. ÇOK ÖNEMLİ: Kendi projenin main.c'sini bu listeden çıkar ki testlerde çakışmasın!
list(REMOVE_ITEM CORE_SOURCES "${CMAKE_SOURCE_DIR}/src/core/main.c")

# Thread'ler için paket bul
find_package(Threads REQUIRED)

file(GLOB TEST_SOURCES *.c)

foreach(test_file ${TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)

    # CORE_SOURCES listesini buraya ekliyoruz (* işareti yerine)
    add_executable(${test_name} ${test_file} ${CORE_SOURCES} ${DSL_SOURCES})

    target_include_directories(${test_name} PRIVATE 
        ${CMAKE_SOURCE_DIR}/include/core
        ${CMAKE_SOURCE_DIR}/include/dsl
        ../external/unity/src
    )

    target_link_libraries(${test_name} PRIVATE 
        unity
        CURL::libcurl
        SQLite::SQLite3
        Threads::Threads # Pthread desteğini buraya ekledik
    )

    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()