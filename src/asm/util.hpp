#pragma once

#include <util.hpp>

/// Used to mark instructions for the python codegen
#define INST void

/// Mark that no padding should be used in marked struct, ever
#define PACKED __attribute__((__packed__))

/// Hoist value from assembly into C/C++ as a function return value, and return
#define RETURN_TRANSIENT(T, format) {volatile T tmp; asm("" : format (tmp)); return tmp;}