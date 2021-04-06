cmake_minimum_required(VERSION 3.1)

project(process-watcher LANGUAGES C)

if (UNIX)
    find_library(NCURSES_LIBRARIES
        NAMES ncurses
        REQUIRED)
    find_library(THREAD_LIBRARIES
        NAMES pthread
        REQUIRED)
    set(LIBRARIES ${THREAD_LIBRARIES} ${NCURSES_LIBRARIES})
elseif(WIN32)
    find_library(PSAPI_LIBRARIES
        NAMES Psapi
        REQUIRED)

    if (NOT PDCURSES_LIB_PATH)
        # search will be running in $PATH environment variable 
        set(PDCURSES_LIB_PATH $ENV{PATH})
    endif()
    if (NOT PDCURSES_INCLUDE_PATH) 
        set(PDCURSES_INCLUDE_PATH $ENV{PATH})
    endif()

    if(PDCURSES_USE_DLL)
        set(PDCURSES_LIB_NAME pdcurses)
        set(PDCURSES_LIB_NAME_DLL ${PDCURSES_LIB_NAME}.dll)
        set(PDCURSES_IMPLIB_NAME "")

        if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
            set(PDCURSES_LIB_NAME_LIB ${PDCURSES_LIB_NAME}.lib)
        else() # TODO: other compilers with .lib? 
            set(PDCURSES_LIB_NAME_LIB ${PDCURSES_LIB_NAME}.a) 
        endif()

        if (EXISTS "${PDCURSES_IMPORTED_LOCATION}/${PDCURSES_LIB_NAME_LIB}")
            set(PDCURSES_IMPLIB_NAME ${PDCURSES_LIB_NAME_LIB})
        endif()
        find_path(PDCURSES_IMPORTED_LOCATION NAMES ${PDCURSES_LIB_NAME_DLL} PATHS "${PDCURSES_LIB_PATH}")
        if (NOT PDCURSES_IMPORTED_LOCATION)
            message(FATAL_ERROR "The library ${PDCURSES_LIB_NAME_DLL} not found in ${PDCURSES_LIB_PATH}/.")
        endif()

        if ("${PDCURSES_IMPLIB_NAME}" STREQUAL "")
            message(FATAL_ERROR "No matching imported files (.lib, .a) for ${PDCURSES_LIB_NAME_DLL} in ${PDCURSES_IMPORTED_LOCATION}/.")
        endif()

        add_library(${PDCURSES_LIB_NAME} SHARED IMPORTED $<TARGET_OBJECTS:${PDCURSES_IMPLIB_NAME}>)
        set_target_properties(${PDCURSES_LIB_NAME} PROPERTIES
            IMPORTED_LOCATION "${PDCURSES_IMPORTED_LOCATION}/${PDCURSES_LIB_NAME_DLL}"
            IMPORTED_IMPLIB "${PDCURSES_IMPORTED_LOCATION}/${PDCURSES_IMPLIB_NAME}"
            INTERFACE_INCLUDE_DIRECTORIES "${PDCURSES_INCLUDE_PATH}")
        
        set(PDCURSES_LIBRARIES ${PDCURSES_LIB_NAME})
    else()
        set(PDCURSES_LIBRARIES pdcurses)
    endif()

    set(ADDITIONAL_INCLUDE_DIRECTORIES ${PDCURSES_INCLUDE_PATH})
    set(LIBRARIES ${PDCURSES_LIBRARIES} ${PSAPI_LIBRARIES})    
endif()

set(SOURCE_FILES
    src/main.c
    src/ioutils.c
    src/process.c
    src/twindow.c
    src/cmdargs.c
    src/keys.c
    src/multithreading.c)
set(PRIVATE_HEADER_FILES
    src/twindow.h
    src/cmdargs.h
    src/keys.h
    src/multithreading.h)
set(PUBLIC_HEADER_FILES
    include/process.h
    include/props.h
    include/ioutils.h)

set(C_PROJECT_LINK_FLAGS
    )
set(C_PROJECT_COMPILE_DEFINITIONS
    -D__BINARY_NAME=\"${PROJECT_NAME}\")

if(WIN32)
    list(APPEND C_PROJECT_COMPILE_DEFINITIONS 
        -D_WIN32_WINNT=0x0601) # Windows 7 and later
endif()

if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(C_PROJECT_COMPILE_FLAGS
         /GL /RTCc /RTCu /RTCs /W4 /std:c11)
    list(APPEND C_PROJECT_COMPILE_DEFINITIONS PDC_DLL_BUILD _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU") # gcc, mingw
    set(C_PROJECT_COMPILE_FLAGS
        -Wextra -Wall -Wshadow -Wcast-align -Wwrite-strings -Wuninitialized -std=gnu11 -Wundef -Wconversion
        -Wunreachable-code -Wno-unknown-pragmas)
endif()

if (TESTS_ENABLED)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        list(APPEND C_PROJECT_COMPILE_FLAGS -g)
        if(UNIX)
            list(APPEND C_PROJECT_COMPILE_FLAGS -fno-omit-frame-pointer -fsanitize=address)
            list(APPEND C_PROJECT_LINK_FLAGS -lasan)
        endif()
    endif()
else()
    if (CMAKE_C_COMPILER_ID STREQUAL "MSVC") 
        list(APPEND C_PROJECT_COMPILE_FLAGS /Ox)
    elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        list(APPEND C_PROJECT_COMPILE_FLAGS -O3)
    endif()
endif()

add_executable(${PROJECT_NAME}
    ${SOURCE_FILES} ${PRIVATE_HEADER_FILES} ${PUBLIC_HEADER_FILES})
target_compile_options(${PROJECT_NAME} PUBLIC ${C_PROJECT_COMPILE_FLAGS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${C_PROJECT_LINK_FLAGS} ${LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${C_PROJECT_COMPILE_DEFINITIONS})
target_include_directories(${PROJECT_NAME} PRIVATE src PUBLIC include ${ADDITIONAL_INCLUDE_DIRECTORIES})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin COMPONENT binary)

if (TESTS_ENABLED) # gcc or mingw
    set(PROJECT_INCLUDE_DIRECTORIES include src) 

    set(PROJECT_TEST_NAME ${PROJECT_NAME}-test)
    enable_testing()

    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        list(APPEND C_PROJECT_COMPILE_FLAGS -g)
    endif()
    if (UNIX)
        list(APPEND C_PROJECT_COMPILE_FLAGS -fno-omit-frame-pointer -fsanitize=address)
        list(APPEND C_PROJECT_LINK_FLAGS -lasan)
    endif ()

    set(TEST_SOURCE_FILES
        tests/main.c
        tests/testing-globals.c
        tests/test-ioutils.c
        tests/test-process.c
        tests/test-cmdargs.c)
    set(TEST_HEADER_FILES
        tests/testing-globals.h)

    list(REMOVE_ITEM SOURCE_FILES src/main.c)
    add_executable(${PROJECT_TEST_NAME}
        ${SOURCE_FILES} ${PRIVATE_HEADER_FILES} ${PUBLIC_HEADER_FILES}
        ${TEST_SOURCE_FILES} ${TEST_HEADER_FILES})
    target_compile_options(${PROJECT_TEST_NAME} PRIVATE ${C_PROJECT_COMPILE_FLAGS})
    target_link_libraries(${PROJECT_TEST_NAME} PRIVATE ${C_PROJECT_LINK_FLAGS} ${LIBRARIES})
    target_compile_definitions(${PROJECT_TEST_NAME} PUBLIC ${C_PROJECT_COMPILE_DEFINITIONS})
    target_include_directories(${PROJECT_TEST_NAME} PRIVATE src PUBLIC  
        ${PROJECT_INCLUDE_DIRECTORIES}
        ${ADDITIONAL_INCLUDE_DIRECTORIES})

    add_test(NAME "${PROJECT_TEST_NAME}" COMMAND ${PROJECT_TEST_NAME})
endif()
