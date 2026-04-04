#pragma once
#include <cstdint>

namespace asmio {

	/// See DWARF specification v5 section 7.5.1, page 199
	/// Indicates the kind of compilation unit
	enum struct DwarfUnit : uint8_t {
		compile       = 0x01,
		type          = 0x02,
		partial       = 0x03,
		skeleton      = 0x04,
		split_compile = 0x05,
		split_type    = 0x06,
	};

}
