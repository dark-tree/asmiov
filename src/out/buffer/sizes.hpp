#pragma once

#include "external.hpp"

namespace asmio {

	enum Size : uint8_t {
		VOID = 0,
		BYTE = 1,
		WORD = 2,
		DWORD = 4,
		QWORD = 8,
		TWORD = 10,
	};

}