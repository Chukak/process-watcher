cmake_minimum_required(VERSION 3.1)

project(system-watcher LANGUAGES C)

set(SOURCE_FILES
    src/main.c
    src/system-watcher-globals.c
    src/process.c)
set(PRIVATE_HEADER_FILES
    src/system-watcher-globals.h)
set(PUBLIC_HEADER_FILES
    include/process.h)

set(C_PROJECT_COMPILE_FLAGS
    -Wextra -Wall -Wshadow -Wcast-align -Wwrite-strings -Wuninitialized -std=gnu11)
set(C_PROJECT_LINK_FLAGS
    )
set(C_PROJECT_COMPILE_DEFINITIONS
    -D_DEFAULT_SOURCE)

if (TESTS_ENABLED AND UNIX)
    list(APPEND C_PROJECT_COMPILE_FLAGS -g -fno-omit-frame-pointer -fsanitize=address)
    list(APPEND C_PROJECT_LINK_FLAGS -lasan)
endif()

add_executable(${PROJECT_NAME}
    ${SOURCE_FILES} ${PRIVATE_HEADER_FILES} ${PUBLIC_HEADER_FILES})
target_compile_options(${PROJECT_NAME} PUBLIC ${C_PROJECT_COMPILE_FLAGS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${C_PROJECT_LINK_FLAGS})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${C_PROJECT_COMPILE_DEFINITIONS})
target_include_directories(${PROJECT_NAME} PRIVATE src PUBLIC include)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin COMPONENT binary)

if (TESTS_ENABLED)
    set(PROJECT_TEST_NAME ${PROJECT_NAME}-test)
    enable_testing()

    if (UNIX)
        list(APPEND C_PROJECT_COMPILE_FLAGS -fno-omit-frame-pointer -fsanitize=address)
        list(APPEND C_PROJECT_LINK_FLAGS -lasan)
    endif ()

    set(TEST_SOURCE_FILES
        tests/main.c
        tests/testing-globals.c
        tests/test-system-watcher-globals.c
        tests/test-process.c)
    set(TEST_HEADER_FILES
        tests/testing-globals.h)

    list(REMOVE_ITEM SOURCE_FILES src/main.c)
    add_executable(${PROJECT_TEST_NAME}
        ${SOURCE_FILES} ${PRIVATE_HEADER_FILES} ${PUBLIC_HEADER_FILES}
        ${TEST_SOURCE_FILES} ${TEST_HEADER_FILES})
    target_compile_options(${PROJECT_TEST_NAME} PRIVATE ${C_PROJECT_COMPILE_FLAGS})
    target_link_libraries(${PROJECT_NAME}-test PRIVATE ${C_PROJECT_LINK_FLAGS})
    target_compile_definitions(${PROJECT_NAME} PUBLIC ${C_PROJECT_COMPILE_DEFINITIONS})
    target_include_directories(${PROJECT_TEST_NAME} PRIVATE src PUBLIC include)
endif()
