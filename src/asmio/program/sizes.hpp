#pragma once

#include <asmio/external.hpp>

namespace asmio {

	enum Size : uint8_t {
		VOID = 0,
		BYTE = 1,     // 8 bits
		WORD = 2,     // 16 bits
		DWORD = 4,    // 32 bits
		QWORD = 8,    // 64 bits
		TWORD = 10,   // 80 bits
		XMMWORD = 16, // 128 bits
	};

}