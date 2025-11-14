#pragma once

#include "external.hpp"
#include "header.hpp"
#include "section.hpp"
#include "segment.hpp"
#include "out/buffer/segmented.hpp"
#include "out/chunk/buffer.hpp"

#define DEFAULT_ELF_MOUNT 0x08048000

namespace asmio {

	enum struct RunResult : uint8_t {
		SUCCESS,     // elf file was executed
		ARGS_ERROR,  // the given arguments are invalid
		MEMFD_ERROR, // memfd failed
		MMAP_ERROR,  // mmap failed
		SEAL_ERROR,  // fcntl failed
		STAT_ERROR,  // fstat failed
		FORK_ERROR,  // fork failed
		EXEC_ERROR,  // file not executable
		WAIT_ERROR,  // waitpid failed
	};

	inline std::ostream& operator<<(std::ostream& os, RunResult c) {
		switch(c) {
			case RunResult::SUCCESS: return os << "SUCCESS";
			case RunResult::ARGS_ERROR: return os << "ARGS_ERROR";
			case RunResult::MEMFD_ERROR: return os << "MEMFD_ERROR";
			case RunResult::SEAL_ERROR: return os << "SEAL_ERROR";
			case RunResult::FORK_ERROR: return os << "FORK_ERROR";
			case RunResult::EXEC_ERROR: return os << "EXEC_ERROR";
			case RunResult::WAIT_ERROR: return os << "WAIT_ERROR";
			default: return os << "UNKNOWN";
		}
	}

	inline uint32_t to_elf_flags(const BufferSegment& segment) {
		uint32_t flags = 0;
		if (segment.flags & BufferSegment::R) flags |= ElfSegmentFlags::R;
		if (segment.flags & BufferSegment::W) flags |= ElfSegmentFlags::W;
		if (segment.flags & BufferSegment::X) flags |= ElfSegmentFlags::X;

		return flags;
	}

	/**
	 * Based on Tool Interface Standard (TIS) Executable and Linking Format (ELF)
	 * Specification (version 1.2), and the ELF man page.
	 *
	 * 1. https://refspecs.linuxfoundation.org/elf/elf.pdf
	 * 2. https://www.man7.org/linux/man-pages/man5/elf.5.html
	 */
	class ElfFile {

		protected:

			static constexpr int ELF_VERSION = 1;

			ChunkBuffer::Ptr root;

		private:

			bool has_sections = false;
			bool has_segments = false;

			ChunkBuffer::Ptr section_headers;
			ChunkBuffer::Ptr segment_headers;
			ChunkBuffer::Ptr segments;
			ChunkBuffer::Ptr sections;
			ChunkBuffer::Ptr section_string_table;

			void define_section(const std::string& name, const ChunkBuffer::Ptr& section, ElfSectionType type, uint32_t link, uint32_t info);
			void define_segment(ElfSegmentType type, uint32_t flags, const ChunkBuffer::Ptr& segment, uint64_t address, uint64_t tail, uint64_t align);

		public:

			ElfFile(ElfMachine machine, uint64_t mount, uint64_t entrypoint);

			/**
			 * Create a new ELF section in the given segment, if no segment is provided the
			 * section will be created in a common off-segment chunk. The backing data buffer is returned.
			 */
			ChunkBuffer::Ptr section(const std::string& name, ElfSectionType type, uint32_t link, uint32_t info, const ChunkBuffer::Ptr& segment = nullptr);

			/**
			 * Create a new ELF segment,
			 * this can then be used to create section in, or used directly as a data buffer.
			 */
			ChunkBuffer::Ptr segment(ElfSegmentType type, uint32_t flags, uint64_t address, uint64_t tail = 0);

	};

	class ElfBuffer : public ElfFile {

		public:

			/**
			 * creates a new ELF buffer
			 * @param segmented content
			 * @param mount page aligned virtual mounting address
			 * @param entrypoint offset in bytes into the content where the execution will begin
			 */
			explicit ElfBuffer(SegmentedBuffer& segmented, uint64_t mount, uint64_t entrypoint);

		public:

			/// Save buffer as a ELF file
			bool save(const char* path) const;

			/// Fork into the in-memory view of the file
			RunResult execute(const char* name, int* status) const;

			/// Fork into the in-memory view of the file, with arguments
			RunResult execute(const char** argv, const char** envp, int* status) const;

	};

	inline ElfBuffer to_elf(SegmentedBuffer& segmented, const Label& entry, uint64_t address = DEFAULT_ELF_MOUNT, const Linkage::Handler& handler = nullptr) {

		// after alignment we will know how big the buffer needs to be
		const size_t page = getpagesize();
		segmented.align(page);
		segmented.link(address, handler);

		if (!segmented.has_label(entry)) {
			throw std::runtime_error {"Entrypoint '" + entry.string() + "' not defined!"};
		}

		BufferMarker entrymark = segmented.get_label(entry);
		uint64_t entrypoint = segmented.get_offset(entrymark);

		return ElfBuffer {segmented, address, entrypoint};
	}

}