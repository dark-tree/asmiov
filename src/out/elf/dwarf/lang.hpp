#pragma once
#include <cstdint>

namespace asmio {

	/// See DWARF specification v5 section 7.12, page 230
	/// Defined DWARF language codes
	enum struct DwarfLang : uint16_t {
		C89            = 0x0001,
		C              = 0x0002,
		Ada83          = 0x0003,
		C_plus_plus    = 0x0004,
		Cobol74        = 0x0005,
		Cobol85        = 0x0006,
		Fortran77      = 0x0007,
		Fortran90      = 0x0008,
		Pascal83       = 0x0009,
		Modula2        = 0x000a,
		Java           = 0x000b,
		C99            = 0x000c,
		Ada95          = 0x000d,
		Fortran95      = 0x000e,
		PLI            = 0x000f,
		ObjC           = 0x0010,
		ObjC_plus_plus = 0x0011,
		UPC            = 0x0012,
		D              = 0x0013,
		Python         = 0x0014,
		OpenCL         = 0x0015,
		Go             = 0x0016,
		Modula3        = 0x0017,
		Haskell        = 0x0018,
		C_plus_plus_03 = 0x0019,
		C_plus_plus_11 = 0x001a,
		OCaml          = 0x001b,
		Rust           = 0x001c,
		C11            = 0x001d,
		Swift          = 0x001e,
		Julia          = 0x001f,
		Dylan          = 0x0020,
		C_plus_plus_14 = 0x0021,
		Fortran03      = 0x0022,
		Fortran08      = 0x0023,
		RenderScript   = 0x0024,
		BLISS          = 0x0025,
	};
	
}
