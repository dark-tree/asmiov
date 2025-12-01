#pragma once

#include <out/chunk/buffer.hpp>

#include "external.hpp"

namespace asmio {

	constexpr uint32_t UNDEFINED_SECTION = 0;

	struct ElfSectionCreateInfo {
		std::function<uint32_t()> link = [] () noexcept { return 0; };
		std::function<uint32_t()> info = [] () noexcept { return 0; };
		ChunkBuffer::Ptr segment = nullptr;

		uint64_t address = 0;
		uint64_t alignment = 0;
		uint64_t entry_size = 0;
		uint64_t flags = 0;
	};

	struct ElfSectionFlags {
		static constexpr uint32_t W = 0b001; ///< Writable section
		static constexpr uint32_t R = 0b010; ///< Readable section
		static constexpr uint32_t X = 0b100; ///< Executable section
	};

	enum struct ElfSectionType : uint32_t {
		NONE     = 0,  ///< Inactive
		PROGBITS = 1,  ///< Information defined by the program, whose format and meaning are determined solely by the program
		SYMTAB   = 2,  ///< Symbol table
		STRTAB   = 3,  ///< String table
		RELA     = 4,  ///< Relocation entries with explicit addends
		HASH     = 5,  ///< Symbol hash table. All objects participating in dynamic linking must contain a symbol hash table.
		DYNAMIC  = 6,  ///< Information for dynamic linking
		NOTE     = 7,  ///< Custom attached metadata
		NOBITS   = 8,  ///< Occupies no space in the file but otherwise resembles SHT_PROGBITS.
		REL      = 9,  ///< Relocation entries without explicit addends
		SHLIB    = 10, ///< Reserved but has unspecified semantics
		DYNSYM   = 11, ///< Minimal set of dynamic linking symbols
	};

	struct PACKED ElfSectionHeader {
		uint32_t name;       ///< Offset into the string table
		ElfSectionType type; ///< Section type
		uint64_t flags;      ///< See ElfSectionFlags
		uint64_t addr;       ///< Virtual address at which the section should reside in memory
		uint64_t offset;     ///< Offset to the first byte of the section
		uint64_t size;       ///< section's size in bytes
		uint32_t link;       ///< Section header table index link
		uint32_t info;       ///< This member holds extra information
		uint64_t addralign;  ///< Alignment should be a positive, integral power of 2, and address must be congruent to 0, modulo the value of alignment
		uint64_t entsize;    ///< Size of one entry for sections where the entries have a fixed size, 0 otherwise
	};

	struct PACKED ElfRelocationInfo {
		uint32_t sym;
		uint32_t type;
	};

	struct PACKED ElfImplicitRelocation {
		uint64_t offset;
		ElfRelocationInfo info;
	};

	struct PACKED ElfExplicitRelocation {
		uint64_t offset;
		ElfRelocationInfo info;
		int64_t addend;
	};

}
