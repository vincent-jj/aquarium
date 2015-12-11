project (Plankton)

cmake_minimum_required(VERSION 2.8)
find_package(SDL REQUIRED)
find_package(SDL_image REQUIRED)

set(CMAKE_BUILD_TYPE Debug)

set(SRC_DIR "src")
set(EXECUTABLE_OUTPUT_PATH build)
set(CMAKE_CXX_FLAGS "-Wall -std=c++11")

file(GLOB_RECURSE Plankton_SOURCES "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE Plankton_HEADERS "${SRC_DIR}/*.hpp")

#Pour si on veut faire des sous-dossiers dans src
set(Plankton_INCLUDE_DIRS "")
foreach(_headerFile ${Plankton_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND Plankton_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES Plankton_INCLUDE_DIRS)

add_executable (Client_Plankton ${Plankton_SOURCES})
target_link_libraries(Client_Plankton pthread SDL SDL_image)
include_directories(${SDL_INCLUDE_DIR} ${SDL_IMAGE_INCLUDE_DIR})
