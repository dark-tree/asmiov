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

}