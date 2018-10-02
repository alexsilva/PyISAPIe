cmake_minimum_required(VERSION 3.5)

project(PyISAPIe)

if(TRACE_LOG)
    add_definitions("-DTRACE_LOG=${TRACE_LOG}")
endif()

# python libs
include_directories(${PYTHON_DIR}/include)
link_directories(${PYTHON_DIR}/libs)

include_directories("Module")

include_directories("../inih/cpp")
include_directories("../inih")

set(SOURCE_FILES
        ../inih/cpp/INIReader.cpp
        ../inih/ini.c
        Main.cpp
        ISAPI.cpp
        Header.cpp
        Interpreter.cpp
        ReadWrite.cpp
        Request.cpp
        Trace.cpp
        Module/Env.cpp
        Module/Module.cpp
        Module/ReadWrite.cpp
        ../PyISAPIe.def)

add_library(PyISAPI SHARED ${SOURCE_FILES})

target_link_libraries(PyISAPI python27)

add_executable(test test.cpp)
target_link_libraries(test PyISAPI)