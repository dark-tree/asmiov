#include "model.hpp"

namespace asmio {

	/*
	 * class ElfModel
	 */

	int64_t ElfModel::LinkInfo::resolve() {
		return section ? pointer->index : value;
	}

	int ElfModel::load_symbols(std::vector<Symbol*>& sorted) const {
		int index = 0;
		int locals = 0;

		std::vector<Symbol*> secondary;

		// we will move all symbols to sorted in the end
		sorted.reserve(symbols.size());

		for (Symbol* symbol : symbols) {
			if (symbol->binding == ElfSymbolBinding::LOCAL) {
				sorted.push_back(symbol);
			} else {
				secondary.push_back(symbol);
			}
		}

		for (Symbol* symbol : sorted) {
			symbol->index = index;
			index ++;
		}

		// return the number of local symbols, this includes the 'null symbol'
		// this is the value that needs to be stored into sh_info of the string table
		locals = index;

		for (Symbol* symbol : secondary) {
			symbol->index = index;
			index ++;

			sorted.push_back(symbol);
		}

		return locals;
	}

	ElfModel::Section* ElfModel::define_section(Section& section) {
		section.index = section_indexer.next();
		return sections.push(section);
	}

	ElfModel::ElfModel() {
		data_buffer_pool = std::make_shared<ChunkBuffer>();
		segment_buffer_pool = data_buffer_pool->chunk();
		section_buffer_pool = data_buffer_pool->chunk();
	}

	ElfModel::Segment* ElfModel::segment(ElfSegmentType type, uint32_t flags, uint64_t address, uint64_t tail) {
		const int alignment = getpagesize();

		Segment segment {};
		segment.type = type;
		segment.flags = flags;
		segment.address = address;
		segment.tail = tail;
		segment.align = alignment;
		segment.buffer = segment_buffer_pool->chunk(alignment);
		return segments.push(segment);
	}

	ElfModel::Section* ElfModel::section(ElfSectionType type, const std::string& name, Segment* segment, uint64_t address, uint64_t alignment, uint64_t entry_size, uint64_t flags) {
		if (!section_string_table) {

			Section null {};
			null.buffer = section_buffer_pool->chunk();
			null_section = define_section(null);

			Section shstrtab {};
			shstrtab.type = ElfSectionType::STRTAB;
			shstrtab.name = section_strings.append(".shstrtab");
			shstrtab.alignment = 1;
			shstrtab.buffer = section_buffer_pool->chunk();
			section_string_table = define_section(shstrtab);
		}

		Section section {};
		section.type = type;
		section.name = section_strings.append(name);
		section.address = address;
		section.alignment = alignment;
		section.entry_size = entry_size;
		section.flags = flags;
		section.buffer = (segment ? segment->buffer : section_buffer_pool)->chunk(alignment);
		return define_section(section);
	}

	ElfModel::Symbol* ElfModel::symbol(ElfSymbolType type, const std::string_view& name, ElfSymbolBinding binding, ElfSymbolVisibility visibility, Section* target, size_t offset, size_t size) {
		if (!symbol_table) {

			Section strings {};
			strings.type = ElfSectionType::STRTAB;
			strings.name = section_strings.append(".strtab");
			strings.alignment = 1;
			strings.buffer = section_buffer_pool->chunk();
			symbol_string_table = define_section(strings);

			Section symtab {};
			symtab.type = ElfSectionType::SYMTAB;
			symtab.name = section_strings.append(".symtab");
			symtab.entry_size = sizeof(ElfSymbol);
			symtab.alignment = 8;
			symtab.link = LinkInfo(symbol_string_table);
			symtab.info = LinkInfo(); // This is set during baking, when we know how many local symbols are there
			symtab.buffer = section_buffer_pool->chunk();
			symbol_table = define_section(symtab);

			Symbol symbol {};
			symbol.section = null_section;
			symbols.push(symbol);
		}

		Symbol symbol {};
		symbol.type = type;
		symbol.binding = binding;
		symbol.visibility = visibility;
		symbol.section = target;
		symbol.offset = offset;
		symbol.size = size;
		symbol.name = symbol_strings.append(name);
		symbol.index = 0; // this is resolved later once the model is baked
		return symbols.push(symbol);
	}

	ElfModel::Relocation* ElfModel::relocation(ElfRelocationType type, Symbol* symbol, Section* target, size_t offset, int64_t addend) {
		if (!target->relocations) {
			auto name = section_strings.read(target->name) + ".rela";

			target->relocations = section(ElfSectionType::RELA, name, nullptr, 0, 8, sizeof(ElfExplicitRelocation));
			target->relocations->link = LinkInfo(symbol_table);
			target->relocations->info = LinkInfo(target);
		}

		Relocation relocation {};
		relocation.type = type;
		relocation.symbol = symbol;
		relocation.section = target->relocations;
		relocation.offset = offset;
		relocation.addend = addend;
		return relocations.push(relocation);
	}

	ObjectFile ElfModel::bake() const {

		// this vector will first contain all local symbols, then
		// all the non-local ones (global and weak).
		std::vector<Symbol*> sorted_symbols;

		if (symbol_table) {
			symbol_table->info = LinkInfo(load_symbols(sorted_symbols));
		}

		if (section_string_table) {
			section_string_table->buffer->write(section_strings.data());
		}

		if (symbol_string_table) {
			symbol_string_table->buffer->write(symbol_strings.data());
		}

		ChunkBuffer elf;

		auto header_buffer = elf.chunk();
		auto segment_headers = elf.chunk();
		auto section_headers = elf.chunk();

		// FIXME this nukes the data_buffer_pool, this method is NOT valid to call from a const context
		elf.adopt(data_buffer_pool);

		header_buffer->link<ElfFileHeader>([=, this] (auto& header) {

			ElfIdentification& ident = header.identification;
			ident.magic[0] = 0x7F;
			ident.magic[1] = 'E';
			ident.magic[2] = 'L';
			ident.magic[3] = 'F';

			// rest of the ELF identifier
			ident.clazz = ElfClass::BIT_64;
			ident.data = ElfData::LSB;
			ident.version = ELF_VERSION;
			ident.abi = ElfAbi::SYSV;
			ident.abi_version = 0;
			memset(ident.pad, 0, sizeof(ident.pad));

			const bool has_ph = !segments.empty();
			const bool has_sh = !sections.empty();

			header.type = type;
			header.machine = machine;
			header.version = ELF_VERSION;
			header.entry = entrypoint;
			header.phoff = has_ph ? segment_headers->offset() : 0;
			header.shoff = has_sh ? section_headers->offset() : 0;
			header.flags = 0;
			header.ehsize = sizeof(header);
			header.phentsize = has_ph ? sizeof(ElfSegmentHeader) : 0;
			header.phnum = segments.size();
			header.shentsize = has_sh ? sizeof(ElfSectionHeader) : 0;
			header.shnum = sections.size();
			header.shstrndx = has_sh ? section_string_table->index : 0;

		});

		for (Segment* segment : segments) {

			segment_headers->link<ElfSegmentHeader>([=] (auto& header) {

				const size_t bytes = segment->buffer->size();

				header.type = segment->type;
				header.flags = segment->flags;
				header.offset = bytes ? segment->buffer->offset() : 0;
				header.vaddr = segment->address;
				header.paddr = 0;
				header.filesz = bytes;
				header.memsz = bytes + segment->tail;
				header.align = segment->align;

			});
		}

		for (Section* section : sections) {

			section_headers->link<ElfSectionHeader>([=] (auto& header) {

				header.name = section->name;
				header.type = section->type;
				header.flags = section->flags;
				header.addr = section->address;
				header.size = section->buffer->size();
				header.offset = header.size ? section->buffer->offset() : 0;
				header.link = section->link.resolve();
				header.info = section->info.resolve();
				header.addralign = section->alignment;
				header.entsize = section->entry_size;

			});
		}

		for (Symbol* symbol : sorted_symbols) {

			ElfSymbol sym {};
			sym.name = symbol->name;
			sym.type = symbol->type;
			sym.binding = symbol->binding;
			sym.visibility = symbol->visibility;
			sym.shndx = symbol->section ? symbol->section->index : null_section->index;
			sym.value = symbol->offset;
			sym.ssize = symbol->size;

			symbol_table->buffer->put<ElfSymbol>(sym);
		}

		for (Relocation* relocation : relocations) {

			ElfExplicitRelocation rela {};
			rela.offset = relocation->offset;
			rela.addend = relocation->addend;
			rela.info.type = relocation->type;
			rela.info.sym = relocation->symbol->index;

			relocation->section->buffer->put<ElfExplicitRelocation>(rela);
		}

		return {elf.bake()};
	}

}