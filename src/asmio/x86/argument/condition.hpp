#pragma once

#include <asmio/external.hpp>

namespace asmio::x86 {

	enum struct SimdCondition : uint8_t {
		EQ  = 0, ///< Equal              (ordered,   non-signaling)
		LT  = 1, ///< Less-than          (ordered,       signaling)
		LE  = 2, ///< Less-than-or-equal (ordered,       signaling)
		RD  = 7, ///< Ordered            (ordered,   non-signaling)
		NEQ = 4, ///< Not-equal          (unordered, non-signaling)
		NLT = 5, ///< Not-less-than      (unordered,     signaling)
		NLE = 6, ///< Not-greater-than   (unordered,     signaling)
		NRD = 3, ///< Unordered          (unordered, non-signaling)
	};

}