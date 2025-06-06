#pragma once

#include "external.hpp"

namespace asmio::elf {

	constexpr const uint32_t UNDEFINED_SECTION = 0;

	struct SectionFlags {
		static constexpr const uint32_t WRITE = 0b001;    // Writable section
		static constexpr const uint32_t ALLOCATE = 0b010; // Readable section
		static constexpr const uint32_t EXECUTE = 0b100;  // Executable section
	};

	enum struct SectionType : uint32_t {
		EMPTY    = 0,  // Inactive
		PROGBITS = 1,  // Information defined by the program, whose format and meaning are determined solely by the program
		SYMTAB   = 2,  // Symbol table
		STRTAB   = 3,  // String table
		RELA     = 4,  // Relocation entries with explicit addends
		HASH     = 5,  // Symbol hash table. All objects participating in dynamic linking must contain a symbol hash table.
		DYNAMIC  = 6,  // Information for dynamic linking
		NOTE     = 7,  // Custom attached metadata
		NOBITS   = 8,  // Occupies no space in the file but otherwise resembles SHT_PROGBITS.
		REL      = 9,  // Relocation entries without explicit addends
		SHLIB    = 10, // Reserved but has unspecified semantics
		DYNSYM   = 11, // Minimal set of dynamic linking symbols
	};

	struct __attribute__((__packed__)) SectionHeader {
		uint32_t name;       // Offset into the string table
		SectionType type;    // Section type
		uint32_t flags;      // See SectionFlags
		uint32_t address;    // Virtual address at which the section should reside in memory
		uint32_t offset;     // Offset to the first byte of the section
		uint32_t size;       // section's size in bytes
		uint32_t link;       // Section header table index link
		uint32_t info;       // This member holds extra information
		uint32_t alignment;  // Alignment should be a positive, integral power of 2, and address must be congruent to 0, modulo the value of alignment
		uint32_t entry_size; // Size of one entry for sections where the entries have a fixed size, 0 otherwise
	};

	struct __attribute__((__packed__)) RelocationInfo {
		uint32_t sym : 24;
		uint8_t type : 8;
	};

	struct __attribute__((__packed__)) ImplicitRelocation { // Elf32_Rel
		uint32_t offset;
		RelocationInfo info;
	};

	struct __attribute__((__packed__)) ExplicitRelocation { // Elf32_Rela
		uint32_t offset;
		RelocationInfo info;
		int32_t addend;
	};

}
