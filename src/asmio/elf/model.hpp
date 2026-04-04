#pragma once

#include <asmio/util/indexer.hpp>
#include <asmio/util/pool.hpp>
#include <asmio/util/strings.hpp>

#include "object.hpp"
#include "header.hpp"
#include "relocation.hpp"
#include "section.hpp"
#include "segment.hpp"
#include "symbol.hpp"

namespace asmio {

	/**
	 * Based on Tool Interface Standard (TIS) Executable and Linking Format (ELF)
	 * Specification (version 1.2), and the ELF man page.
	 *
	 * 1. https://refspecs.linuxfoundation.org/elf/elf.pdf
	 * 2. https://www.man7.org/linux/man-pages/man5/elf.5.html
	 * 3. https://flint.cs.yale.edu/cs422/doc/ELF_Format.pdf
	 */
	class ElfModel {

		public:

			struct Section;

			struct Segment {
				ElfSegmentType type = ElfSegmentType::NONE;
				uint32_t flags = 0;
				uint64_t address = 0;
				uint64_t tail = 0;
				uint64_t align = 0;
				ChunkBuffer::Ptr buffer = nullptr;
			};

			struct LinkInfo {
				bool section;
				union {
					Section* pointer;
					int64_t value;
				};

				constexpr LinkInfo(Section* section) noexcept : section(true), pointer(section) {}
				constexpr LinkInfo(int64_t value) noexcept : section(false), value(value) {}
				constexpr LinkInfo() noexcept : section(false), value(0) {}

				int64_t resolve();
			};

			struct Section {
				ElfSectionType type = ElfSectionType::NONE;
				uint32_t name = 0;
				uint64_t address = 0;
				uint64_t alignment = 0;
				uint64_t entry_size = 0;
				uint64_t flags = 0;
				LinkInfo link {};
				LinkInfo info {};
				Section* relocations = nullptr;
				ChunkBuffer::Ptr buffer = nullptr;
				int index = -1;
			};

			struct Symbol {
				ElfSymbolType type = ElfSymbolType::NOTYPE;
				ElfSymbolBinding binding = ElfSymbolBinding::LOCAL;
				ElfSymbolVisibility visibility = ElfSymbolVisibility::DEFAULT;
				Section* section = nullptr;
				size_t offset = 0;
				size_t size = 0;
				uint32_t name = 0;
				int index = 0;
			};

			struct Relocation {
				Symbol* symbol;
				ElfRelocationType type;
				Section* section;
				size_t offset;
				int64_t addend;
			};

		private:

			static constexpr int ELF_VERSION = 1;

			Indexer<int> section_indexer;

			Strings symbol_strings;
			Strings section_strings;

			Section* null_section = nullptr;
			Section* symbol_string_table = nullptr;
			Section* section_string_table = nullptr;
			Section* symbol_table = nullptr;

			Pool<Segment> segments {16};
			Pool<Section> sections {16};
			Pool<Symbol> symbols {128};
			Pool<Relocation> relocations {128};

			ChunkBuffer::Ptr data_buffer_pool;
			ChunkBuffer::Ptr segment_buffer_pool;
			ChunkBuffer::Ptr section_buffer_pool;

			int load_symbols(std::vector<Symbol*>& symbols) const;
			Section* define_section(Section& section);

		public:

			ElfMachine machine = ElfMachine::NONE;
			ElfType type = ElfType::NONE;
			uint64_t entrypoint = 0;

			ElfModel();

			/**
			 * Create a new ELF segment,
			 * this can then be used to create section in, or used directly as a data buffer.
			 */
			Segment* segment(ElfSegmentType type, uint32_t flags, uint64_t address, uint64_t tail = 0);

			/**
			 * Create a new ELF section in the given segment, if no segment is provided the
			 * section will be created in a common off-segment chunk. The backing data buffer is returned.
			 */
			Section* section(ElfSectionType type, const std::string& name, Segment* segment, uint64_t address = 0, uint64_t alignment = 1, uint64_t entry_size = 0, uint64_t flags = 0);

			/**
			 * Create a new ELF symbol,
			 * local and global symbols can be defined in any order.
			 */
			Symbol* symbol(ElfSymbolType type, const std::string_view& name, ElfSymbolBinding binding, ElfSymbolVisibility visibility, Section* section, size_t offset, size_t size);

			/**
			 * Define an ELF relocation entry for a specific symbol in the target buffer,
			 * offset is the section offset in bytes or virtual address for executables and shared objects.
			 */
			Relocation* relocation(ElfRelocationType type, Symbol* symbol, Section* target, size_t offset, int64_t addend = 0);

			/**
			 * Bakes the current model into a read-only buffer
			 * that can be saved to file or executed in memory.
			 */
			ObjectFile bake() const;

	};

}
