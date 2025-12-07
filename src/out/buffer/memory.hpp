#pragma once

#include <external.hpp>
#include <out/elf/symbol.hpp>

namespace asmio {

	struct MemoryFlags {

		bool r : 1;
		bool w : 1;
		bool x : 1;

		constexpr bool operator==(const MemoryFlags& flags) const {
			return r == flags.r && w == flags.w && x == flags.x;
		}

		constexpr MemoryFlags operator|(const MemoryFlags& flags) const {
			return { r || flags.r, w || flags.w, x || flags.x };
		}

		constexpr MemoryFlags operator&(const MemoryFlags& flags) const {
			return { r && flags.r, w && flags.w, x && flags.x };
		}

		MemoryFlags& operator|=(const MemoryFlags& flags) {
			return *this = *this | flags;
		}

		MemoryFlags& operator&=(const MemoryFlags& flags) {
			return *this = *this & flags;
		}

		/// Convert to mprotect() flags
		int to_mprotect() const;

		/// Convert to ElfSegmentFlags flags
		uint32_t to_elf_segment() const;

		/// Convert to ElfSectionFlags flags
		uint32_t to_elf_section() const;

		/// Convert to matching ElfSymbolType
		ElfSymbolType to_elf_symbol() const;

	};

	struct MemoryFlag {

		// those should really be in the MemoryFlags struct but C++ hates being usable
		static constexpr MemoryFlags NONE { false, false, false };
		static constexpr MemoryFlags R { true, false, false };
		static constexpr MemoryFlags W { false, true, false };
		static constexpr MemoryFlags X { false, false, true };

	};

	static_assert(sizeof(MemoryFlags) == 1);

}