cmake_minimum_required(VERSION 3.26)
project(ASMIOV C CXX ASM)

set(CMAKE_CXX_FLAGS "-no-pie")
set(CMAKE_C_FLAGS "-no-pie")
set(CMAKE_ASM_FLAGS "-no-pie")
set(CMAKE_CXX_STANDARD 20)

set(ASM_SOURCES
		src/asm/x86/switch.s
)

set(CPP_SOURCES
		src/asm/x86/writer.cpp
		src/asm/x86/address.cpp
		src/asm/x86/emitter.cpp
		src/asm/x86/instructions/cpu.cpp
		src/asm/x86/instructions/fpu.cpp
		src/tasml/tokenizer.cpp
)

add_library(asmiov ${ASM_SOURCES} ${CPP_SOURCES})

include_directories(test src/ .)
add_executable(test test/tests.cpp)
target_link_libraries(test PRIVATE asmiov)

include_directories(tasml src/ .)
add_executable(tasml src/tasml/tasml.cpp)
target_link_libraries(test PRIVATE asmiov)

set_source_files_properties(${ASM_SOURCES} PROPERTIES LANGUAGE ASM)