#pragma once

#include "external.hpp"

namespace asmio::elf {

	struct SegmentFlags {
		static constexpr uint32_t X = 0b001; // Executable segment
		static constexpr uint32_t W = 0b010; // Writable segment
		static constexpr uint32_t R = 0b100; // Readable segment
	};

	enum struct SegmentType : uint32_t {
		EMPTY   = 0, // Inactive
		LOAD    = 1, // Loadable segment
		DYNAMIC = 2, // Dynamic linking information
		INTERP  = 3, // Location and size of a null-terminated path name to invoke as an interpreter
		NOTE    = 4, // Custom attached metadata
		SHLIB   = 5, // Reserved but has unspecified semantics
		PHDR    = 6  // Specifies the location and size of the program header table itself
	};

	struct __attribute__((__packed__)) SegmentHeader {
		SegmentType type;          // Segment type
		uint32_t flags;            // See SegmentFlags
		uint64_t offset;           // Offset to the first byte of the segment
		uint64_t virtual_address;  // Virtual address at which the segment should reside in memory
		uint64_t physical_address; // On systems for which physical addressing is relevant, physical address at which the segment should reside in memory
		uint64_t file_size;        // Number of bytes in the file image of the segment
		uint64_t memory_size;      // Number of bytes in the memory image of the segment
		uint64_t alignment;        // Alignment should be a positive, integral power of 2, and virtual_address should equal offset, modulo alignment
	};

}