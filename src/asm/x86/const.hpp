#pragma once

#include "external.hpp"

namespace asmio::x86 {

	/// no offset, eg. [REG]
	constexpr uint8_t MOD_NONE     = 0b00;

	/// 16bit offset
	constexpr uint8_t MOD_BYTE     = 0b01;

	/// 32bit offset
	constexpr uint8_t MOD_QUAD     = 0b10;

	/// reg-to-reg, as in "mov EAX, EBX"
	constexpr uint8_t MOD_SHORT    = 0b11;

	/// placed in the r/m field along with mod set to anything but MOD_SHORT to enable the SIB byte
	constexpr uint8_t RM_SIB       = 0b100;

	/// placed in the SIB byte's 'index' to mark the lack of index (makes SIB just a base holder)
	constexpr uint8_t NO_SIB_INDEX = 0b100;

	/// MUST be used when NO_SIB_INDEX is used
	constexpr uint8_t NO_SIB_SCALE = 0b00;

	/// rotate left (without carry)
	constexpr uint8_t INST_ROL     = 0b000;

	/// rotate right (without carry)
	constexpr uint8_t INST_ROR     = 0b001;

	/// rotate left (through carry)
	constexpr uint8_t INST_RCL     = 0b010;

	/// rotate right (through carry)
	constexpr uint8_t INST_RCR     = 0b011;

	/// shift left
	constexpr uint8_t INST_SHL     = 0b100;

	/// shift right
	constexpr uint8_t INST_SHR     = 0b101;

	/// arithmetic shift right (preserve sign)
	constexpr uint8_t INST_SAR     = 0b111;

	/// Move Data From String to String
	constexpr uint8_t INST_MOVS    = 0b10100100;

	/// Input String from Port
	constexpr uint8_t INST_INS     = 0b01101100;

	/// Output String to Port
	constexpr uint8_t INST_OUTS    = 0b01101110;

	/// Compare Strings
	constexpr uint8_t INST_CMPS    = 0b10100110;

	/// Scan String
	constexpr uint8_t INST_SCAS    = 0b10101110;

	/// Load String
	constexpr uint8_t INST_LODS    = 0b10101100;

	/// Store String
	constexpr uint8_t INST_STOS    = 0b10101010;

	/// used for an absolute value reference
	constexpr uint8_t NO_BASE      = 0b101;

	/// The fixed bit pattern of the REX prefix
	constexpr uint8_t REX_PREFIX   = 0b0100'0000;

	/// REX W bit, used to switch addressing to 64 bit
	constexpr uint8_t REX_BIT_W    = 0b1000;

	/// REX R bit, used as the bit 4 of MODRM.reg
	constexpr uint8_t REX_BIT_R    = 0b0100;

	/// REX X bit, used as the bit 4 of SIB.index
	constexpr uint8_t REX_BIT_X    = 0b0010;

	/// REX B bit, used as the bit 4 of MODRM.rm SIB.base
	constexpr uint8_t REX_BIT_B    = 0b0001;

	/// Often used as a prefix byte before instructions
	constexpr uint8_t LONG_OPCODE  = 0x0F;

}