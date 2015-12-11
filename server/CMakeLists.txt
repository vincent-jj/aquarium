project (Plankton)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_BUILD_TYPE Debug)

set(SRC_DIR "src")
set(TST_DIR "tst")
set(EXECUTABLE_OUTPUT_PATH build)

file(GLOB_RECURSE Plankton_SOURCES "${SRC_DIR}/*.c")
file(GLOB_RECURSE Plankton_HEADERS "${SRC_DIR}/*.h")

file(GLOB_RECURSE test_source "${TST_DIR}/*.c")


#Pour si on veut faire des sous-dossiers dans src
set(Plankton_INCLUDE_DIRS "")
foreach(_headerFile ${Plankton_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND Plankton_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES Plankton_INCLUDE_DIRS)
#include_directories(${Plankton_INCLUDE_DIRS})

set(TST_SOURCE ${test_source} ${Plankton_SOURCES})
list(REMOVE_ITEM TST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c)

add_executable (Plantkon ${Plankton_SOURCES})
add_executable (Plantkon_test ${TST_SOURCE})
