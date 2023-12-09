#pragma once

#include <cinttypes>

namespace asmio::x86 {

	// no offset, eg. [REG]
	constexpr const uint8_t MOD_NONE  = 0b00;

	// 16bit offset
	constexpr const uint8_t MOD_BYTE  = 0b01;

	// 32bit offset
	constexpr const uint8_t MOD_QUAD  = 0b10;

	// reg-to-reg, as in "mov EAX, EBX"
	constexpr const uint8_t MOD_SHORT = 0b11;

	// placed in the r/m field along with mod set to MOD_SHORT to enable the SIB byte
	constexpr const uint8_t RM_SIB       = 0b100;

	// placed in the SIB byte's 'index' to mark the lack of index (makes SIB just a base holder)
	constexpr const uint8_t NO_SIB_INDEX = 0b100;

	// MUST be used when NO_SIB_INDEX is used
	constexpr const uint8_t NO_SIB_SCALE = 0b00;

}