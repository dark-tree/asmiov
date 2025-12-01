#pragma once

#include "external.hpp"
#include "header.hpp"
#include "section.hpp"
#include "segment.hpp"
#include "symbol.hpp"
#include "out/buffer/segmented.hpp"
#include "out/chunk/buffer.hpp"

#define DEFAULT_ELF_MOUNT 0x08048000

namespace asmio {

	template <auto V>
	constexpr static auto supply = [] noexcept { return V; };

	enum struct RunStatus {
		SUCCESS,     ///< elf file was executed
		ARGS_ERROR,  ///< the given arguments are invalid
		MEMFD_ERROR, ///< memfd failed
		MMAP_ERROR,  ///< mmap failed
		SEAL_ERROR,  ///< fcntl failed
		STAT_ERROR,  ///< fstat failed
		FORK_ERROR,  ///< fork failed
		EXEC_ERROR,  ///< file not executable
		WAIT_ERROR,  ///< waitpid failed
	};

	struct RunResult {
		RunResult(RunStatus type)
			: type(type), status(0) {
		}

		RunResult(int status)
			: type(RunStatus::SUCCESS), status(status) {
		}

		const RunStatus type;
		const int status;
	};

	std::ostream& operator<<(std::ostream& os, RunStatus c);
	std::ostream& operator<<(std::ostream& os, const RunResult& result);

	/**
	 * Based on Tool Interface Standard (TIS) Executable and Linking Format (ELF)
	 * Specification (version 1.2), and the ELF man page.
	 *
	 * 1. https://refspecs.linuxfoundation.org/elf/elf.pdf
	 * 2. https://www.man7.org/linux/man-pages/man5/elf.5.html
	 */
	class ElfFile {

		public:

			struct IndexedChunk {
				ChunkBuffer::Ptr data;
				int index;
			};

		protected:

			static constexpr int ELF_VERSION = 1;

			ChunkBuffer::Ptr root;

		private:

			bool has_sections = false;
			bool has_segments = false;
			bool has_symbols = false;

			ChunkBuffer::Ptr section_headers;
			ChunkBuffer::Ptr segment_headers;
			ChunkBuffer::Ptr segments;
			ChunkBuffer::Ptr sections;
			ChunkBuffer::Ptr section_string_table;

			ChunkBuffer::Ptr symbol_strings;
			ChunkBuffer::Ptr local_symbols;
			ChunkBuffer::Ptr other_symbols;

			std::unordered_map<std::string, IndexedChunk> section_map;

			int define_section(const std::string& name, const ChunkBuffer::Ptr& section, ElfSectionType type, const ElfSectionCreateInfo& info);
			int define_segment(ElfSegmentType type, uint32_t flags, const ChunkBuffer::Ptr& segment, uint64_t address, uint64_t tail, uint64_t align);

		public:

			ElfFile(ElfMachine machine, uint64_t mount, uint64_t entrypoint);

			/**
			 * Create a new ELF section in the given segment, if no segment is provided the
			 * section will be created in a common off-segment chunk. The backing data buffer is returned.
			 */
			IndexedChunk section(const std::string& name, ElfSectionType type, const ElfSectionCreateInfo& info);

			/**
			 * Create a new ELF segment,
			 * this can then be used to create section in, or used directly as a data buffer.
			 */
			IndexedChunk segment(ElfSegmentType type, uint32_t flags, uint64_t address, uint64_t tail = 0);

			/**
			 * Create a new ELF symbol,
			 * local and global symbols can be defined in any order.
			 */
			void symbol(const std::string& name, ElfSymbolType type, ElfSymbolBinding binding, ElfSymbolVisibility visibility, const IndexedChunk& target, size_t offset, size_t size);

		public:

			/**
			 * Save the ELF to an executable file,
			 * if that is possible the file is given the execute permission.
			 */
			bool save(const std::string& path) const;

			/**
			 * Serialize the ELF file to a byte buffer,
			 * if you wish to save the file use save() instead.
			 */
			std::vector<uint8_t> bytes() const;

			/**
			 * Fork into the in-memory view of the file,
			 * environ is inherited from the calling process.
			 */
			RunResult execute(const char* name) const;

			/**
			 * Fork into the in-memory view of the file, with arguments
			 * to inherit environ pass it as the second argument.
			 */
			RunResult execute(const char** argv, const char** envp) const;

	};

	inline ElfFile to_elf(SegmentedBuffer& segmented, const Label& entry, uint64_t address = DEFAULT_ELF_MOUNT, const Linkage::Handler& handler = nullptr) {

		static auto to_segment_flags = [] (const BufferSegment& segment) -> uint32_t {
			uint32_t flags = 0;
			if (segment.flags & BufferSegment::R) flags |= ElfSegmentFlags::R;
			if (segment.flags & BufferSegment::W) flags |= ElfSegmentFlags::W;
			if (segment.flags & BufferSegment::X) flags |= ElfSegmentFlags::X;

			return flags;
		};

		static auto to_section_flags = [] (const BufferSegment& segment) -> uint32_t {
			uint32_t flags = 0;
			if (segment.flags & BufferSegment::R) flags |= ElfSectionFlags::R;
			if (segment.flags & BufferSegment::W) flags |= ElfSectionFlags::W;
			if (segment.flags & BufferSegment::X) flags |= ElfSectionFlags::X;

			return flags;
		};

		// after alignment we will know how big the buffer needs to be
		const size_t page = getpagesize();
		segmented.align(page);
		segmented.link(address, handler);

		if (!segmented.has_label(entry)) {
			throw std::runtime_error {"Entrypoint '" + entry.string() + "' not defined!"};
		}

		BufferMarker entrymark = segmented.get_label(entry);
		uint64_t entrypoint = segmented.get_offset(entrymark);

		ElfFile elf {segmented.elf_machine, address, entrypoint};
		std::vector<ExportSymbol> symbols = segmented.resolved_exports();

		bool create_sections = true;
		std::unordered_map<int, int> section_map;

		for (const BufferSegment& segment : segmented.segments()) {
			if (segment.empty()) {
				continue;
			}

			auto segment_chunk = elf.segment(ElfSegmentType::LOAD, to_segment_flags(segment), address, segment.tail);
			auto section_chunk = segment_chunk;

			// create intermediate section between the segment and that data we want to save
			if (create_sections) {
				ElfSectionCreateInfo info {};
				info.address = address;
				info.flags = to_section_flags(segment);
				info.segment = segment_chunk.data;

				section_chunk = elf.section(segment.name, ElfSectionType::PROGBITS, info);
			}

			section_chunk.data->write(segment.buffer);
			segment_chunk.data->push(segment.tail);

			address += segment.size();
		}

		return elf;
	}

}
