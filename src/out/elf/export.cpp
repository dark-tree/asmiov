#include "export.hpp"

#include "out/buffer/segmented.hpp"

namespace asmio {

	static void export_line_data(ElfFile& elf, SegmentedBuffer& segmented) {

		// don't create the line emitter if we have no lines
		if (segmented.locations().empty()) {
			return;
		}

		auto emitter = elf.line_emitter();

		const DwarfDir root = emitter->add_directory("/");
		std::vector<DwarfFile> files;

		for (const auto& file : segmented.files()) {
			files.emplace_back(emitter->add_file(root, file));
		}

		for (const auto& line : segmented.locations()) {
			size_t address = segmented.get_offset(line.marker);
			emitter->set_mapping(address, files[line.file], line.line, line.column);
		}

		emitter->end_sequence();

	}

	ElfFile to_elf(SegmentedBuffer& segmented, const Label& entry, uint64_t address, const Linkage::Handler& handler) {

		struct MappingInfo {
			int section;
			ElfSymbolType content;
		};

		// after alignment, we will know how big the buffer needs to be
		const size_t page = getpagesize();
		segmented.align(page);
		segmented.link(address, handler);

		uint64_t entrypoint = 0;
		ElfType type = ElfType::REL;
		bool create_sections = true;

		// if we have an entrypoint create an executable file
		if (!entry.empty()) {
			if (!segmented.has_label(entry)) {
				throw std::runtime_error {"Entrypoint '" + entry.string() + "' not defined!"};
			}

			entrypoint = segmented.get_offset(segmented.get_label(entry));
			type = ElfType::EXEC;
		}

		ElfFile elf {segmented.elf_machine, type, address + entrypoint};
		std::unordered_map<int, MappingInfo> section_map;

		for (const BufferSegment& segment : segmented.segments()) {
			if (segment.empty()) {
				continue;
			}

			auto segment_chunk = elf.segment(ElfSegmentType::LOAD, segment.flags.to_elf_segment(), address + segment.start, segment.tail);
			auto section_chunk = segment_chunk;

			// create intermediate section between the segment and that data we want to save
			if (create_sections) {
				ElfSectionCreateInfo info {};
				info.address = address;
				info.flags = segment.flags.to_elf_section();
				info.segment = segment_chunk.data;

				section_chunk = elf.section(segment.name, ElfSectionType::PROGBITS, info);

				const ElfSymbolType content = segment.flags.to_elf_symbol();
				section_map[segment.index] = {section_chunk.index, content};
			}

			section_chunk.data->write(segment.buffer);
			segment_chunk.data->push(segment.tail);
		}

		for (const ExportSymbol& symbol : segmented.exports()) {
			const Label& label = symbol.label;

			if (!label.is_text()) {
				continue;
			}

			BufferMarker marker = segmented.get_label(label);
			MappingInfo info = section_map[marker.section];

			ElfSymbolBinding binding = ElfSymbolBinding::GLOBAL;
			ElfSymbolVisibility visibility = ElfSymbolVisibility::DEFAULT;

			switch (symbol.type) {

				case ExportSymbol::PRIVATE:
					binding = ElfSymbolBinding::LOCAL;
					visibility = ElfSymbolVisibility::HIDDEN;
					break;

				case ExportSymbol::PUBLIC:
					binding = ElfSymbolBinding::GLOBAL;
					visibility = ElfSymbolVisibility::PROTECTED;
					break;

				case ExportSymbol::WEAK:
					binding = ElfSymbolBinding::WEAK;
					visibility = ElfSymbolVisibility::PROTECTED;
					break;

			}

			elf.symbol(label.string(), info.content, binding, visibility, info.section, marker.offset, symbol.size);
		}

		export_line_data(elf, segmented);

		return elf;
	}

}