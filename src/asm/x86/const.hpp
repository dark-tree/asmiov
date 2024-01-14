#pragma once

#include "external.hpp"

namespace asmio::x86 {

	// no offset, eg. [REG]
	constexpr const uint8_t MOD_NONE     = 0b00;

	// 16bit offset
	constexpr const uint8_t MOD_BYTE     = 0b01;

	// 32bit offset
	constexpr const uint8_t MOD_QUAD     = 0b10;

	// reg-to-reg, as in "mov EAX, EBX"
	constexpr const uint8_t MOD_SHORT    = 0b11;

	// placed in the r/m field along with mod set to anything but MOD_SHORT to enable the SIB byte
	constexpr const uint8_t RM_SIB       = 0b100;

	// placed in the SIB byte's 'index' to mark the lack of index (makes SIB just a base holder)
	constexpr const uint8_t NO_SIB_INDEX = 0b100;

	// MUST be used when NO_SIB_INDEX is used
	constexpr const uint8_t NO_SIB_SCALE = 0b00;

	// rotate left (without carry)
	constexpr const uint8_t INST_ROL     = 0b000;

	// rotate right (without carry)
	constexpr const uint8_t INST_ROR     = 0b001;

	// rotate left (through carry)
	constexpr const uint8_t INST_RCL     = 0b010;

	// rotate right (through carry)
	constexpr const uint8_t INST_RCR     = 0b011;

	// shift left
	constexpr const uint8_t INST_SHL     = 0b100;

	// shift right
	constexpr const uint8_t INST_SHR     = 0b101;

	// arithmetic shift right (preserve sign)
	constexpr const uint8_t INST_SAR     = 0b111;

	// Move Data From String to String
	constexpr const uint8_t INST_MOVS    = 0b10100100;

	// Input String from Port
	constexpr const uint8_t INST_INS     = 0b01101100;

	// Output String to Port
	constexpr const uint8_t INST_OUTS    = 0b01101110;

	// Compare Strings
	constexpr const uint8_t INST_CMPS    = 0b10100110;

	// Scan String
	constexpr const uint8_t INST_SCAS    = 0b10101110;

	// Load String
	constexpr const uint8_t INST_LODS    = 0b10101100;

	// Store String
	constexpr const uint8_t INST_STOS    = 0b10101010;

	// used for an absolute value reference
	constexpr const uint8_t NO_BASE      = 0b101;

}