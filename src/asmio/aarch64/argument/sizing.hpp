#pragma once

#include <asmio/external.hpp>

enum struct Sizing : uint8_t {
	UB = 0b000, ///< unsigned byte
	UH = 0b001, ///< unsigned word
	UW = 0b010, ///< unsigned dword
	UX = 0b011, ///< unsinged qword
	SB = 0b100, ///< signed byte
	SH = 0b101, ///< signed word
	SW = 0b110, ///< signed dword
	SX = 0b111, ///< signed qword
};