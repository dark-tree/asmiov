#pragma once

#include <external.hpp>

namespace asmio {

	/// See DWARF specification v5 section 7.8, page 227
	/// DWARF type encoding attribute value
	enum struct DwarfEncoding : uint8_t {
		ADDRESS         = 0x01,
		BOOLEAN         = 0x02,
		COMPLEX_FLOAT   = 0x03,
		FLOAT           = 0x04,
		SIGNED          = 0x05,
		SIGNED_CHAR     = 0x06,
		UNSIGNED        = 0x07,
		UNSIGNED_CHAR   = 0x08,
		IMAGINARY_FLOAT = 0x09,
		PACKED_DECIMAL  = 0x0a,
		NUMERIC_STRING  = 0x0b,
		EDITED          = 0x0c,
		SIGNED_FIXED    = 0x0d,
		UNSIGNED_FIXED  = 0x0e,
		DECIMAL_FLOAT   = 0x0f,
		UTF             = 0x10,
		UCS             = 0x11,
		ASCII           = 0x12,
	};

}
