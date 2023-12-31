#include "address.hpp"

namespace asmio::x86 {

	void assertValidScale(Registry registry, uint8_t scale) {
		if (registry.is(ESP) && registry.size == DWORD) {
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

	Location operator + (ScaledRegistry index, Label label) {
		return Location {index} + label.c_str();
	}

	Location operator + (Registry base, Registry index) {
		return base + (index * 1);
	}

	Location ref(Location location) {
		return location.ref();
	}

}