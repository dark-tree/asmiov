#pragma once

#include "external.hpp"
#include "registry.hpp"
#include "asm/x86/const.hpp"

namespace asmio::x86 {

	/// Check if the register-scale combination is valid
	constexpr void check_valid_scale(Registry registry, uint8_t scale) {

		// use .reg not .low() on purpose, as only RSP/EBP can't be
		// used in index, R12 (0b1100) is fine as the REX extension is decoded
		if (registry.reg == NO_SIB_INDEX) {
			throw std::runtime_error {"Invalid operand, RSP/ESP can't be used as scaled index!"};
		}

		if (std::popcount(scale) > 1 || scale > 0b1111) {
			throw std::runtime_error {"A registry can only be scaled by one of (1, 2, 4, 8) in expression!"};
		}

	}

	/// Represents the scaled register part in intel addressing, such as EAX * 2, EAX * 4
	struct ScaledRegistry {

		const Registry registry;
		const uint8_t scale : 4; // the scale, as integer

		constexpr explicit ScaledRegistry(Registry registry, uint8_t scale)
		: registry(registry), scale(scale) {
			check_valid_scale(registry, scale);
		}

		constexpr bool operator == (ScaledRegistry const& other) const {
			return (registry == other.registry) && (scale == other.scale);
		}

	};

	/// Create a ScaledRegistry object for used in Location
	constexpr ScaledRegistry operator * (Registry registry, uint8_t scale) {
		return ScaledRegistry {registry, scale};
	}

}