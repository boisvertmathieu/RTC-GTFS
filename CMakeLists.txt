cmake_minimum_required(VERSION 3.4)
project(rtc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}) #for the executable
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/libs) #for static library
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/libs) #for shared library

add_library(rtcGraphe STATIC src/graphe.cpp)

link_directories(${PROJECT_SOURCE_DIR}/libs)

add_executable(main src/main.cpp)
target_link_libraries(main rtcNetwork rtcGraphe rtcReader)
