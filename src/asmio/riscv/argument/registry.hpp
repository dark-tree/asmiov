#pragma once

#include <asmio/external.hpp>

namespace asmio::riscv {

	struct PACKED Registry {

		public:

			const uint8_t reg; // registry Risc-V code

		public:

			constexpr Registry(uint8_t reg) noexcept : reg(reg) {}

	};

	/// Reference arbitrary register, 'number' MUST be in range [0, 31]
	constexpr Registry X(uint8_t number) {
		return {static_cast<uint8_t>(number & 0b11111)};
	}

	// general purpose QWORD registers
	constexpr Registry X0 = X(0); // always equal 0
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
	constexpr Registry X31 = X(31);

	// register name aliases
	constexpr Registry RA = X1;   // return address
	constexpr Registry SP = X2;   // stack pointer
	constexpr Registry GP = X3;   // global data pointer (read-only)
	constexpr Registry TP = X4;   // thread data pointer (read-only)
	constexpr Registry T0 = X5;   // temporary register
	constexpr Registry T1 = X6;   // temporary register
	constexpr Registry T2 = X7;   // temporary register
	constexpr Registry S0 = X8;   // saved register
	constexpr Registry S1 = X9;   // saved register
	constexpr Registry A0 = X10;  // argument register
	constexpr Registry A1 = X11;  // argument register
	constexpr Registry A2 = X12;  // argument register
	constexpr Registry A3 = X13;  // argument register
	constexpr Registry A4 = X14;  // argument register
	constexpr Registry A5 = X15;  // argument register
	constexpr Registry A6 = X16;  // argument register
	constexpr Registry A7 = X17;  // argument register
	constexpr Registry S2 = X18;  // saved register
	constexpr Registry S3 = X19;  // saved register
	constexpr Registry S4 = X20;  // saved register
	constexpr Registry S5 = X21;  // saved register
	constexpr Registry S6 = X22;  // saved register
	constexpr Registry S7 = X23;  // saved register
	constexpr Registry S8 = X24;  // saved register
	constexpr Registry S9 = X25;  // saved register
	constexpr Registry S10 = X26; // saved register
	constexpr Registry S11 = X27; // saved register
	constexpr Registry T3 = X28;  // temporary register
	constexpr Registry T4 = X29;  // temporary register
	constexpr Registry T5 = X30;  // temporary register
	constexpr Registry T6 = X31;  // temporary register

}