#pragma once

#include <util.hpp>

/// Used to mark instructions for the python codegen
#define INST void

/// Hoist value from assembly into C/C++ as a function return value, and return
#define RETURN_TRANSIENT(T, format) {volatile T tmp; asm("" : format (tmp)); return tmp;}