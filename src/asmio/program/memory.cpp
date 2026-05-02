#include "memory.hpp"

#include <asmio/elf/section.hpp>
#include <asmio/elf/segment.hpp>

namespace asmio {

	/*
	 * class MemoryFlag
	 */

	int MemoryFlags::to_mprotect() const {
		int protect = 0;
		if (r) protect |= 0x1; // PROT_READ
		if (w) protect |= 0x2; // PROT_WRITE
		if (x) protect |= 0x4; // PROT_EXEC
		return protect;
	}

	uint32_t MemoryFlags::to_win32() const {
		// this gotta be the dumbest way to implement memory permissions...
		if (x && !r && !w) return 0x10; // PAGE_EXECUTE
		if (x && r && !w) return 0x20; // PAGE_EXECUTE_READ
		if (x && w) return 0x40; // PAGE_EXECUTE_READWRITE (write implies read)
		if (!x && !r && !w) return 0x01; // PAGE_NOACCESS
		if (!x && r && !w) return 0x02; // PAGE_READONLY
		if (!x && w) return 0x04; // PAGE_READWRITE (write implies read)

		// unreachable
		return 0;
	}

	uint32_t MemoryFlags::to_elf_segment() const {
		uint32_t flags = 0;
		if (r) flags |= ElfSegmentFlags::R;
		if (w) flags |= ElfSegmentFlags::W;
		if (x) flags |= ElfSegmentFlags::X;
		return flags;
	}

	uint32_t MemoryFlags::to_elf_section() const {
		uint32_t flags = 0;
		if (r) flags |= ElfSectionFlags::R;
		if (w) flags |= ElfSectionFlags::W;
		if (x) flags |= ElfSectionFlags::X;
		return flags;
	}

	ElfSymbolType MemoryFlags::to_elf_symbol() const {
		return x ? ElfSymbolType::FUNC : ElfSymbolType::OBJECT;
	}

}

