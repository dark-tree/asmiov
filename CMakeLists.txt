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