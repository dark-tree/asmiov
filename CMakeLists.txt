cmake_minimum_required(VERSION 3.26)
project(LAIO C CXX ASM)

set(CMAKE_CXX_FLAGS "-no-pie")
set(CMAKE_C_FLAGS "-no-pie")
set(CMAKE_ASM_FLAGS "-no-pie")
set(CMAKE_CXX_STANDARD 20)

set(ASM_SOURCES
        src/switch.s
)

add_executable(LAIO
        src/main.cpp
        ${ASM_SOURCES}
)

set_source_files_properties(${ASM_SOURCES} PROPERTIES LANGUAGE ASM)