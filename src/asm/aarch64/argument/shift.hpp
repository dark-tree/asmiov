#pragma once

#include "external.hpp"

enum struct ShiftType : uint8_t {
	LSL = 0b00, // shift left
	LSR = 0b01, // shift right
	ASR = 0b10, // arithmetic shift right
	ROR = 0b11  // rotate right
};