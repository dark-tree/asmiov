cmake_minimum_required(VERSION 3.26)
project(LAIO C CXX ASM)

set(CMAKE_CXX_FLAGS "-no-pie")
set(CMAKE_C_FLAGS "-no-pie")
set(CMAKE_ASM_FLAGS "-no-pie")
set(CMAKE_CXX_STANDARD 20)

set(ASM_SOURCES
        src/switch.s
)

set(CPP_SOURCES
        src/writer.cpp
        src/address.cpp
        src/instructions/cpu.cpp
        src/instructions/fpu.cpp
)

include_directories(LAIO
        src/
        .
)

add_executable(LAIO
        test/tests.cpp
        ${ASM_SOURCES}
        ${CPP_SOURCES}
)

set_source_files_properties(${ASM_SOURCES} PROPERTIES LANGUAGE ASM)