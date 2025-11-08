#pragma once

#include "external.hpp"
#include "../../util.hpp"
#include "out/buffer/sizes.hpp"

namespace asmio::arm {

	struct PACKED Registry {

		public:

			/// Register properties
			enum Flag : uint8_t {
				NONE        = 0b0000,
				ZERO        = 0b0001, // this is the special 'zero' register
				STACK       = 0b0010, // this is SP
				GENERAL     = 0b0100, // this is any general-purpose register
			};

			const uint8_t size : 4; // size in bytes
			const uint8_t flag : 4; // additional flags
			const uint8_t reg;      // registry ARM code

			constexpr Registry(uint8_t size, uint8_t reg, uint8_t flag)
			: size(size), flag(flag), reg(reg) {}

		private:

		public:


			/// Check if the register has a given flag
			constexpr bool is(Flag mask) const {
				return (flag & mask) != 0;
			}

			constexpr bool wide() {
				return size == QWORD;
			}

	};

	/// Reference arbitrary 32bit register, 'number' MUST be in range [0, 30]
	constexpr Registry W(uint8_t number) {
		return {DWORD, static_cast<uint8_t>(number & 0b11111), Registry::GENERAL};
	}

	/// Reference arbitrary 64bit register, 'number' MUST be in range [0, 30]
	constexpr Registry X(uint8_t number) {
		return {QWORD, static_cast<uint8_t>(number & 0b11111), Registry::GENERAL};
	}

	// special registers
	constexpr Registry UNSET {VOID,  31, Registry::NONE};
	constexpr Registry WZR   {DWORD, 31, Registry::ZERO | Registry::GENERAL};
	constexpr Registry XZR   {QWORD, 31, Registry::ZERO | Registry::GENERAL};
	constexpr Registry SP    {QWORD, 31, Registry::STACK};
	constexpr Registry LR = X(30);
	constexpr Registry FP = X(29);

	// general purpose QWORD registers
	constexpr Registry X0 = X(0);
	constexpr Registry X1 = X(1);
	constexpr Registry X2 = X(2);
	constexpr Registry X3 = X(3);
	constexpr Registry X4 = X(4);
	constexpr Registry X5 = X(5);
	constexpr Registry X6 = X(6);
	constexpr Registry X7 = X(7);
	constexpr Registry X8 = X(8);
	constexpr Registry X9 = X(9);
	constexpr Registry X10 = X(10);
	constexpr Registry X11 = X(11);
	constexpr Registry X12 = X(12);
	constexpr Registry X13 = X(13);
	constexpr Registry X14 = X(14);
	constexpr Registry X15 = X(15);
	constexpr Registry X16 = X(16);
	constexpr Registry X17 = X(17);
	constexpr Registry X18 = X(18);
	constexpr Registry X19 = X(19);
	constexpr Registry X20 = X(20);
	constexpr Registry X21 = X(21);
	constexpr Registry X22 = X(22);
	constexpr Registry X23 = X(23);
	constexpr Registry X24 = X(24);
	constexpr Registry X25 = X(25);
	constexpr Registry X26 = X(26);
	constexpr Registry X27 = X(27);
	constexpr Registry X28 = X(28);
	constexpr Registry X29 = X(29);
	constexpr Registry X30 = X(30);

	// general purpose DWARD registers
	constexpr Registry W0 = W(0);
	constexpr Registry W1 = W(1);
	constexpr Registry W2 = W(2);
	constexpr Registry W3 = W(3);
	constexpr Registry W4 = W(4);
	constexpr Registry W5 = W(5);
	constexpr Registry W6 = W(6);
	constexpr Registry W7 = W(7);
	constexpr Registry W8 = W(8);
	constexpr Registry W9 = W(9);
	constexpr Registry W10 = W(10);
	constexpr Registry W11 = W(11);
	constexpr Registry W12 = W(12);
	constexpr Registry W13 = W(13);
	constexpr Registry W14 = W(14);
	constexpr Registry W15 = W(15);
	constexpr Registry W16 = W(16);
	constexpr Registry W17 = W(17);
	constexpr Registry W18 = W(18);
	constexpr Registry W19 = W(19);
	constexpr Registry W20 = W(20);
	constexpr Registry W21 = W(21);
	constexpr Registry W22 = W(22);
	constexpr Registry W23 = W(23);
	constexpr Registry W24 = W(24);
	constexpr Registry W25 = W(25);
	constexpr Registry W26 = W(26);
	constexpr Registry W27 = W(27);
	constexpr Registry W28 = W(28);
	constexpr Registry W29 = W(29);
	constexpr Registry W30 = W(30);

}