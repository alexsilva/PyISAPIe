cmake_minimum_required(VERSION 3.5)

project(PyISAPIe)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/${CMAKE_BUILD_TYPE})

add_subdirectory(Source/PyISAPIe)
