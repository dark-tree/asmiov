#pragma once

enum struct AddType : uint8_t {
	UXTB = 0b000, // add unsigned byte
	UXTH = 0b001, // add unsigned word
	UXTW = 0b010, // add unsigned dword
	UXTX = 0b011, // add unsinged qword
	SXTB = 0b100, // add signed byte
	SXTH = 0b101, // add signed word
	SXTW = 0b110, // add signed dword
	SXTX = 0b111, // add signed qword
};