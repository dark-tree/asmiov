#pragma once

#include "buffer.hpp"

namespace asmio {

	/*
	 * https://en.wikipedia.org/wiki/LEB128
	 */

	struct UnsignedLeb128 {
		static void encode(ChunkBuffer& buffer, uint64_t value);
	};

	struct SignedLeb128 {
		static void encode(ChunkBuffer& buffer, int64_t signed_value);
	};

}