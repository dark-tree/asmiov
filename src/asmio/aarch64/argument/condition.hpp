#pragma once

#include <asmio/external.hpp>

namespace asmio::arm {

	enum struct Condition : uint8_t {
		EQ = 0b0000, // equal
		NE = 0b0001, // not equal
		CS = 0b0010, // carry set
		CC = 0b0011, // carry clear
		MI = 0b0100, // minus
		PL = 0b0101, // plus
		VS = 0b0110, // overflow set
		VC = 0b0111, // overflow clear
		HI = 0b1000, // higher
		LS = 0b1001, // lower or same
		GE = 0b1010, // greater or equal
		LT = 0b1011, // less than
		GT = 0b1100, // greater than
		LE = 0b1101, // less or equal
		AL = 0b1110, // always
		NV = 0b1111, // never, unused and treated the same as AL

		GEU = CC, // greater or equal unsigned
		LTU = CS, // less than unsigned
		GTU = HI, // greater than unsigned
		LEU = LS, // less or equal unsigned
	};

	/**
	 * Invert the condition as if it had been negated,
	 * the always true condition can't be inverted as there is no functional 'never' condition code.
	 */
	constexpr Condition invert(Condition condition) {
		if (condition == Condition::AL) throw std::runtime_error {"The 'always' condition can't be inverted, as there is no functional 'never' condition!"};
		return static_cast<Condition>(static_cast<uint32_t>(condition) ^ 1);
	}

}