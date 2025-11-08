#pragma once

#include <util.hpp>

struct StaticBlock {

	int operator +(const std::function<void()>& block) {
		block();
		return 0;
	}

};

#define JOIN_PARTS_RESOLVED(first, second) first##second

/// Allows one to merge two tokens, even macro values
#define JOIN_PARTS(first, second) JOIN_PARTS_RESOLVED(first, second)

/// Used to mark prefixes for the python codegen
#define PREFIX BufferWriter&

/// Used to mark instructions for the python codegen
#define INST void

/// Mark that no padding should be used in marked struct, ever
#define PACKED __attribute__((__packed__))

/// Hoist value from assembly into C/C++ as a function return value, and return
#define RETURN_TRANSIENT(T, format) {volatile T tmp; asm("" : format (tmp)); return tmp;}

/// Allow for creating global scope hooks
#define STATIC_BLOCK static inline auto JOIN_PARTS(__static_, __LINE__) = (StaticBlock {}) + [] () noexcept(false) -> void