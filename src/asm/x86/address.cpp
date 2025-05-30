#include "address.hpp"

namespace asmio::x86 {

	void assertValidScale(Registry registry, uint8_t scale) {

		// use .reg not .low() on purpose, as only RSP/EBP can't be
		// used in index, R12 (0b1100) is fine as the REX extension is decoded
		if (registry.reg == NO_SIB_INDEX) {
			throw std::runtime_error {"Invalid operand, RSP/ESP can't be used as scaled index!"};
		}

		if (std::popcount(scale) > 1 || scale > 0b1111) {
			throw std::runtime_error {"A registry can only be scaled by one of (1, 2, 4, 8) in expression!"};
		}
	}

	void assertNonReferential(Location const* location, const char* why) {
		if (location->reference) {
			throw std::runtime_error {why};
		}
	}

	Location operator + (Registry registry, int offset) {
		return Location {registry, UNSET, 1, offset, nullptr, registry.size, false};
	}

	Location operator - (Registry registry, int offset) {
		return Location {registry, UNSET, 1, -offset, nullptr, registry.size, false};
	}

	Location operator + (Registry registry, Label label) {
		return Location {registry, UNSET, 1, 0, label.c_str(), registry.size, false};
	}

	ScaledRegistry operator * (Registry registry, uint8_t scale) {
		return ScaledRegistry {registry, scale};
	}

	Location operator + (Registry base, ScaledRegistry index) {

		uint8_t size = base.size;

		if (index.registry.size != VOID) {
			size = index.registry.size;
		}

		// TODO this is already checked in put_inst_std, but maybe we should check it here while we're at it?
		// if (size != VOID && index.registry.size != size) {
		// 	throw std::runtime_error {"Invalid operand, base and index registers need to be of the same size"};
		// }

		return Location {base, index.registry, index.scale, 0, nullptr, size, false};
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

		const bool ia = a.is_indeterminate();
		const bool ib = b.is_indeterminate();

		if (a.is_immediate() && b.is_immediate()) {
			throw std::runtime_error {"Both operands can't be immediate"};
		}

		if (a.is_memory() && b.is_memory()) {
			throw std::runtime_error {"Both operands can't reference memory"};
		}

		if (ia && ib) {
			throw std::runtime_error {"Both operands can't be of indeterminate size"};
		}

		if ((!ia && !ib) && (a.size != b.size)) {
			throw std::runtime_error {"Both operands need to be of the same size"};
		}

		const bool any_rex = a.base.is(Registry::REX) | b.base.is(Registry::REX) | a.index.is(Registry::REX) | b.index.is(Registry::REX);
		const bool any_high = a.base.is(Registry::HIGH_BYTE) | b.base.is(Registry::HIGH_BYTE) | a.index.is(Registry::HIGH_BYTE) | b.index.is(Registry::HIGH_BYTE);

		if (any_rex && any_high) {
			throw std::runtime_error {"Can't use high byte register in the same instruction as an extended register"};
		}

		return ia ? b.size : a.size;
}

}