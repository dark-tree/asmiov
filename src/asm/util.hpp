#pragma once

#include <util.hpp>

struct StaticBlock {

	int operator +(const std::function<void()>& block) {
		block();
		return 0;
	}

};

/// Used to mark prefixes for the python codegen
#define PREFIX BufferWriter&

/// Used to mark instructions for the python codegen
#define INST void

/// Allow for creating global scope hooks
#define STATIC_BLOCK static inline auto JOIN_PARTS(__static_, __LINE__) = (StaticBlock {}) + [] () noexcept(false) -> void