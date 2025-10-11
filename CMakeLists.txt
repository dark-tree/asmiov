cmake_minimum_required(VERSION 3.31)
project(ASMIOV C CXX)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FetchContent_Declare(
		vstl
		GIT_REPOSITORY https://github.com/magistermaks/lib-vstl
		GIT_TAG v3.4
)

FetchContent_MakeAvailable(vstl)

set(ASMIO_SOURCES
		src/out/buffer/segmented.cpp
		src/out/buffer/executable.cpp
		src/out/buffer/writer.cpp
		src/out/buffer/label.cpp

		src/asm/x86/writer.cpp
		src/asm/x86/emitter.cpp
		src/asm/x86/argument/location.cpp
		src/asm/x86/instructions/cpu.cpp
		src/asm/x86/instructions/fpu.cpp

		src/tasml/tokenizer.cpp
		src/tasml/token.cpp
)

include_directories(src/ . ${vstl_SOURCE_DIR})
add_library(asmiov ${ASMIO_SOURCES})

add_executable(tasml src/tasml/tasml.cpp)
target_link_libraries(tasml PRIVATE asmiov)

add_executable(test test/tests.cpp)
target_link_libraries(test PRIVATE asmiov)