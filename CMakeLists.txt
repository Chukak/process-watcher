cmake_minimum_required(VERSION 3.10)

project(system-watcher LANGUAGES C)

set(SOURCE_FILES
    src/main.c
    src/system-watcher-globals.c)
set(PRIVATE_HEADER_FILES
    src/system-watcher-globals.h)
set(PUBLIC_HEADER_FILES
    )

set(C_PROJECT_COMPILE_FLAGS
    )
set(C_PROJECT_LINK_FLAGS
    )

add_executable(${PROJECT_NAME}
    ${SOURCE_FILES} ${PRIVATE_HEADER_FILES} ${PUBLIC_HEADER_FILES})
set_target_properties(${PROJECT_NAME}
    PROPERTIES COMPILE FLAGS ${C_PROJECT_COMPILE_FLAGS}
    PROPERTIES LINK_FLAGS ${C_PROJECT_LINK_FLAGS})
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin COMPONENT binary)

if (TESTS_ENABLED)
    enable_testing()

    set(TEST_SOURCE_FILES
        tests/testing-globals.c)
    set(TEST_HEADER_FILES
        tests/testing-globals.h)

    list(REMOVE_ITEM SOURCE_FILES src/main.c)
    add_executable(${PROJECT_NAME}-test
        ${SOURCE_FILES} ${PRIVATE_HEADER_FILES} ${PUBLIC_HEADER_FILES}
        ${TEST_SOURCE_FILES} ${TEST_HEADER_FILES})
    set_target_properties(${PROJECT_NAME}-test
        PROPERTIES COMPILE FLAGS ${C_PROJECT_COMPILE_FLAGS}
        PROPERTIES LINK_FLAGS ${C_PROJECT_LINK_FLAGS})
    target_include_directories(${PROJECT_NAME}-test PRIVATE src PUBLIC include)
endif()
