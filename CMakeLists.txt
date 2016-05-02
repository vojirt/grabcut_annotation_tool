cmake_minimum_required(VERSION 2.8)

project(grabcut_annotation)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic -O3 -Wno-long-long -fno-omit-frame-pointer")

FIND_PACKAGE( OpenCV REQUIRED )
link_directories ( ${OpenCV_LIB_DIR} )
MESSAGE(STATUS "OpenCV_LIB_DIR: ${OpenCV_LIB_DIR}")

include_directories ( ${OpenCV_INCLUDE_DIRS} )
MESSAGE(STATUS "OpenCV_INCLUDE_DIRS: ${OpenCV_INCLUDE_DIRS}")

INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR}/)
include_directories(${CMAKE_SOURCE_DIR}/ ${CMAKE_SOURCE_DIR}/src/)

file(GLOB SOURCES src/*.cpp src/*.h src/*.hpp)
add_executable(grabcut_annotation main.cpp vot.hpp ${SOURCES})
target_link_libraries(grabcut_annotation ${OpenCV_LIBS})

add_executable(annotation_vis main_vis.cpp)
target_link_libraries(annotation_vis ${OpenCV_LIBS})
