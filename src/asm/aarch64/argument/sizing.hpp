#pragma once

enum struct Sizing : uint8_t {
	UB = 0b000, ///< add unsigned byte
	UH = 0b001, ///< add unsigned word
	UW = 0b010, ///< add unsigned dword
	UX = 0b011, ///< add unsinged qword
	SB = 0b100, ///< add signed byte
	SH = 0b101, ///< add signed word
	SW = 0b110, ///< add signed dword
	SX = 0b111, ///< add signed qword
};