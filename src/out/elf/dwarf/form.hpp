#pragma once

namespace asmio {

	/// See DWARF specification v5 section 7.5.6, page 220
	/// DWARF data encoding description
	enum struct DwarfForm : uint8_t {
		addr           = 0x01,
		block2         = 0x03,
		block4         = 0x04,
		data2          = 0x05,
		data4          = 0x06,
		data8          = 0x07,
		string         = 0x08,
		block          = 0x09,
		block1         = 0x0a,
		data1          = 0x0b,
		flag           = 0x0c,
		sdata          = 0x0d,
		strp           = 0x0e,
		udata          = 0x0f,
		ref_addr       = 0x10,
		ref1           = 0x11,
		ref2           = 0x12,
		ref4           = 0x13,
		ref8           = 0x14,
		ref_udata      = 0x15,
		indirect       = 0x16,
		sec_offset     = 0x17,
		exprloc        = 0x18,
		flag_present   = 0x19,
		strx           = 0x1a,
		addrx          = 0x1b,
		ref_sup4       = 0x1c,
		strp_sup       = 0x1d,
		data16         = 0x1e,
		line_strp      = 0x1f,
		ref_sig8       = 0x20,
		implicit_const = 0x21,
		loclistx       = 0x22,
		rnglistx       = 0x23,
		ref_sup8       = 0x24,
		strx1          = 0x25,
		strx2          = 0x26,
		strx3          = 0x27,
		strx4          = 0x28,
		addrx1         = 0x29,
		addrx2         = 0x2a,
		addrx3         = 0x2b,
		addrx4         = 0x2c,

	};
	
}