#include "memory.hpp"
#include <out/elf/section.hpp>
#include <out/elf/segment.hpp>

namespace asmio {

	/*
	 * class MemoryFlag
	 */

	int MemoryFlags::to_mprotect() const {
		int protect = 0;
		if (r) protect |= PROT_READ;
		if (w) protect |= PROT_WRITE;
		if (x) protect |= PROT_EXEC;
		return protect;
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

