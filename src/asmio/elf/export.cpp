#include "export.hpp"

#include "dwarf/lines.hpp"
#include <asmio/program/segmented.hpp>
#include <asmio/util/platform.hpp>

namespace asmio {

	struct MappingInfo {
		ElfModel::Section* section;
		ElfSymbolType content;
	};

	static void export_line_data(ElfModel& model, SegmentedBuffer& segmented) {

		// don't create the line emitter if we have no lines
		if (segmented.locations().empty()) {
			return;
		}

		auto section = model.section(ElfSectionType::PROGBITS, DwarfLineEmitter::SECTION, nullptr, {});
		DwarfLineEmitter emitter {section->buffer, 8};

		const DwarfDir root = emitter.add_directory("/");
		std::vector<DwarfFile> files;

		for (const auto& file : segmented.files()) {
			files.emplace_back(emitter.add_file(root, file));
		}

		for (const auto& line : segmented.locations()) {
			size_t address = segmented.get_offset(line.marker);
			emitter.set_mapping(address, files[line.file], line.line, line.column);
		}

		emitter.end_sequence();
	}

	static void export_defined_symbols(ElfModel& model, SegmentedBuffer& segmented, std::unordered_map<int, MappingInfo>& section_map) {
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

			model.symbol(info.content, label.view(), binding, visibility, info.section, marker.offset, symbol.size);
		}
	}

	static void export_undefined_symbols(ElfModel& model, std::vector<Linkage>& symbols, std::unordered_map<int, MappingInfo>& section_map) {
		for (auto& linkage : symbols) {
			auto mapping = section_map[linkage.target.section];
			ElfModel::Symbol* symbol = model.symbol(mapping.content, linkage.label.view(), ElfSymbolBinding::GLOBAL, ElfSymbolVisibility::DEFAULT, nullptr, 0, 0);
			model.relocation(linkage.type->relocation, symbol, mapping.section, linkage.target.offset, linkage.addend);
		}
	}

	ElfModel to_elf(SegmentedBuffer& segmented, const Label& entry, uint64_t address, const LinkReporter& handler) {

		// after alignment, we will know how big the buffer needs to be
		const size_t page = page_size();
		segmented.align(page);
		auto unresolved = segmented.link(address, handler);

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

		ElfModel model {};
		std::unordered_map<int, MappingInfo> section_map;

		model.type = type;
		model.entrypoint = address + entrypoint;
		model.machine = segmented.elf_machine;

		for (const BufferSegment& segment : segmented.segments()) {
			if (segment.empty()) {
				continue;
			}

			auto* elf_segment = model.segment(ElfSegmentType::LOAD, segment.flags.to_elf_segment(), address + segment.start, segment.tail);
			auto elf_section_buffer = elf_segment->buffer;

			// create intermediate section between the segment and that data we want to save
			if (create_sections) {
				auto* elf_section = model.section(ElfSectionType::PROGBITS, segment.name, elf_segment, address, 1, 0, segment.flags.to_elf_section());
				elf_section_buffer = elf_section->buffer;

				const ElfSymbolType resident = segment.flags.to_elf_symbol();
				section_map[segment.index] = {elf_section, resident};
			}

			elf_section_buffer->write(segment.buffer);
			elf_segment->buffer->push(segment.tail);
		}

		if (create_sections) {
			export_defined_symbols(model, segmented, section_map);
			export_undefined_symbols(model, unresolved, section_map);
			export_line_data(model, segmented);
		}

		return model;
	}

}
