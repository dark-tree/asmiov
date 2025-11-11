#pragma once

#include "external.hpp"

namespace asmio::arm {

	enum struct Condition : uint8_t {
		EQ = 0b0000, NE = 0b0001,
		CS = 0b0010, CC = 0b0011,
		MI = 0b0100, PL = 0b0101,
		VS = 0b0110, VC = 0b0111,
		HI = 0b1000, LS = 0b1001,
		GE = 0b1010, LT = 0b1011,
		GT = 0b1100, LE = 0b1101,

		// the NV condition code is unused and treated the same as AL
		AL = 0b1110, NV = 0b1111,
	};

	/**
	 * Invert the condition as if it had been negated,
	 * always true condition can't be inverted as AArch64 dropped support for the 'never' condition code.
	 */
	constexpr Condition invert(Condition condition) {
		if (condition == Condition::AL) throw std::runtime_error {"The 'always' condition can't be inverted, as there is no 'never' condition!"};
		return (Condition) (uint32_t(condition) ^ 1);
	}

}