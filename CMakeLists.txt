cmake_minimum_required(VERSION 3.31)
project(ASMIOV C CXX)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Python3 COMPONENTS Interpreter)

# Use the mold linker automatically for GCC if available,
# as it is faster, more modern, and produces better errors.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	find_program(MOLD_PROGRAM mold)

	if (MOLD_PROGRAM)
		message(STATUS "The MOLD linker was found: ${MOLD_PROGRAM}")
		set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=mold")
		set(CMAKE_SHARED_LINKER_FLAGS "-fuse-ld=mold")
	else()
		message(STATUS "The MOLD linker was NOT found!")
	endif()
endif()

FetchContent_Declare(
		vstl
		GIT_REPOSITORY https://github.com/magistermaks/lib-vstl
		GIT_TAG ba434bf32d08a1d0b3679154aa07cad6d15fbb2e
)

FetchContent_MakeAvailable(vstl)

set(ASMIO_WRITERS
		src/asm/x86/writer.hpp
)

set(ASMIO_BRIDGES "${ASMIO_WRITERS}")

list(TRANSFORM ASMIO_BRIDGES REPLACE "src/asm/(.*)/writer.hpp" "${PROJECT_BINARY_DIR}/src/generated/\\1.hpp")
list(TRANSFORM ASMIO_WRITERS PREPEND "${PROJECT_SOURCE_DIR}/")

add_custom_command(
		CODEGEN
		OUTPUT ${ASMIO_BRIDGES}
		COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/util/ingen.py ${ASMIO_WRITERS}
		DEPENDS ${ASMIO_WRITERS}
		DEPENDS ${PROJECT_SOURCE_DIR}/util/ingen.py
)

set(ASMIO_SOURCES
		${ASMIO_BRIDGES}

		src/out/buffer/segmented.cpp
		src/out/buffer/executable.cpp
		src/out/buffer/writer.cpp
		src/out/buffer/label.cpp

		src/asm/module.cpp

		src/asm/x86/writer.cpp
		src/asm/x86/module.cpp
		src/asm/x86/argument/location.cpp
		src/asm/x86/instructions/cpu.cpp
		src/asm/x86/instructions/fpu.cpp

		src/asm/aarch64/writer.cpp
		src/asm/aarch64/instructions/basic.cpp
		src/asm/aarch64/instructions/branch.cpp

		src/tasml/tokenizer.cpp
		src/tasml/token.cpp
)

include_directories(src/ . ${vstl_SOURCE_DIR} "${PROJECT_BINARY_DIR}/src")
add_library(asmiov ${ASMIO_SOURCES})

add_executable(tasml src/tasml/tasml.cpp)
target_link_libraries(tasml PRIVATE asmiov)

add_executable(test test/x86.cpp test/aarch64.cpp)
target_link_libraries(test PRIVATE asmiov)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(asmiov PUBLIC
			-Wall
			-Wextra
			-pedantic

			# Errors
			-Werror=return-type         # Fail if when return in non-void function is missing
			-Werror=return-local-addr   # Fail if a pointer to a local variable is returned
			-Werror=init-self           # Fail if a class member is initialized with itself
			-Werror=uninitialized       # Fail if a uninitialized variables is used
			-Werror=switch

			# Warnings
			-Wnoexcept                  # Warn if the lack of noexcept disables some optimizations
			-Wunreachable-code          # Warn if a section of code is unreachable
			-Wparentheses               # Warn if there is a high chance we forgot about some parentheses
			-Wlogical-op                # Warn if there is a high chance we messed up the logical operator
			-Wdisabled-optimization     # Warn if you managed to confuse the compiler :skull:

			# Ignore some warnings that I deemed stupid
			-Wno-unused-parameter       # Not really an error to ignore something, we may enable it later
			-Wno-variadic-macros        # Allow so called 'variadic macros'
			-Wno-misleading-indentation # Don't complain about non-issues such as "if (thing) do();"
	)
endif()

install(TARGETS tasml)