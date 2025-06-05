
#include "location.hpp"

namespace asmio::x86 {

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