#include "address.hpp"

namespace asmio::x86 {

	void assertValidScale(Registry registry, uint8_t scale) {
		if (registry.is(ESP)) {
			throw std::runtime_error {"The ESP registry can't be scaled!"};
		}

		if (std::popcount(scale) > 1 || scale > 0b1111) {
			throw std::runtime_error {"A registry can only be scaled by one of (1, 2, 4, 8) in expression!"};
		}
	}

	void assertNonReferencial(Location const* location, const char* why) {
		if (location->reference) {
			throw std::runtime_error {why};
		}
	}

	Location operator + (Registry registry, int offset) {
		return Location {registry, UNSET, 1, offset, nullptr, DWORD, false};
	}

	Location operator - (Registry registry, int offset) {
		return Location {registry, UNSET, 1, -offset, nullptr, DWORD, false};
	}

	Location operator + (Registry registry, Label label) {
		return Location {registry, UNSET, 1, 0, label.c_str(), DWORD, false};
	}

	ScaledRegistry operator * (Registry registry, uint8_t scale) {
		return ScaledRegistry {registry, scale};
	}

	Location operator + (Registry base, ScaledRegistry index) {
		return Location {base, index.registry, index.scale, 0, nullptr, DWORD, false};
	}

	Location operator + (ScaledRegistry index, int offset) {
		return Location {index} + offset;
	}

	Location operator - (ScaledRegistry index, int offset) {
		return Location {index} - offset;
	}

	Location operator + (ScaledRegistry index, Label label) {
		return Location {index} + label.c_str();
	}

	Location operator + (Registry base, Registry index) {
		return base + (index * 1);
	}

	Location ref(Location location) {
		return location.ref();
	}

	uint8_t pair_size(const Location& a, const Location& b) {
		if (a.size == b.size) {
			return a.size;
		}

		const bool immediate_a = a.is_immediate();
		const bool immediate_b = b.is_immediate();

		if (immediate_a && immediate_b) {
			throw std::runtime_error {"Both operands can't be immediate"};
		}

		if (a.is_memory() && b.is_memory()) {
			throw std::runtime_error {"Both operands can't reference memory"};
		}

		if (b.is_simple() || immediate_a) {
			return b.size;
		}

		if (a.is_simple() || immediate_b) {
			return a.size;
		}

		// how did we get here?
		throw std::runtime_error {"Invalid operands for reasons undefined"};
	}

}