#pragma once

#include "external.hpp"

enum struct ShiftType : uint8_t {
	LSL = 0b00,
	LSR = 0b01,
	ASR = 0b10,
	ROR = 0b11
};