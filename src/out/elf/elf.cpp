
#include "elf.hpp"

#include <filesystem>

namespace asmio {

	std::ostream& operator<<(std::ostream& os, RunStatus c) {
		switch (c) {
			case RunStatus::SUCCESS: return os << "SUCCESS";
			case RunStatus::ARGS_ERROR: return os << "ARGS_ERROR";
			case RunStatus::MEMFD_ERROR: return os << "MEMFD_ERROR";
			case RunStatus::SEAL_ERROR: return os << "SEAL_ERROR";
			case RunStatus::FORK_ERROR: return os << "FORK_ERROR";
			case RunStatus::EXEC_ERROR: return os << "EXEC_ERROR";
			case RunStatus::WAIT_ERROR: return os << "WAIT_ERROR";
			default: return os << "UNKNOWN";
		}
	}

	std::ostream& operator<<(std::ostream& os, const RunResult& result) {
		return os << "RunResult{status=" << result.type << ", return=" << result.status << "}";
	}

	/*
	 * class ElfFile
	 */

	int ElfFile::define_section(const std::string& name, const ChunkBuffer::Ptr& section, ElfSectionType type, const ElfSectionCreateInfo& info) {

		auto chunk = section_headers->chunk("shdr");
		int string = section_string_table->bytes();

		chunk->link<ElfSectionHeader>([=, info = ElfSectionCreateInfo(info)] (auto& header) {

			header.name = string;
			header.type = type;
			header.flags = info.flags;
			header.addr = info.address;

			if (section) {
				header.offset = section->offset();
				header.size = section->size();
			} else {
				header.offset = 0;
				header.size = 0;
			}

			header.link = info.link();
			header.info = info.info();
			header.addralign = info.alignment;
			header.entsize = info.entry_size;

		});

		section_string_table->write(name);
		return chunk->index();
	}

	int ElfFile::define_segment(ElfSegmentType type, uint32_t flags, const ChunkBuffer::Ptr& segment, uint64_t address, uint64_t tail, uint64_t align) {
		auto chunk = segment_headers->chunk("phdr");

		chunk->link<ElfSegmentHeader>([=] (auto& header) {

			const size_t bytes = segment == nullptr ? 0 : segment->size();

			header.type = type;
			header.flags = flags;
			header.offset = segment == nullptr ? 0 : segment->offset();
			header.vaddr = address;
			header.paddr = 0;
			header.filesz = bytes;
			header.memsz = bytes + tail;
			header.align = align;

		});

		return chunk->index();
	}

	ElfFile::ElfFile(ElfMachine machine, ElfType type, uint64_t entrypoint) {
		root = std::make_shared<ChunkBuffer>();

		auto chunk = root->chunk("ehdr");

		section_headers = root->chunk("shdrs");
		segment_headers = root->chunk("phdrs");
		segments = root->chunk("segments");
		sections = root->chunk("sections");
		section_string_table = sections->chunk("shstrtab");

		chunk->link<ElfFileHeader>([=, this] (auto& header) {

			auto& ident = header.identification;

			// ELF magic number
			ident.magic[0] = 0x7F;
			ident.magic[1] = 'E';
			ident.magic[2] = 'L';
			ident.magic[3] = 'F';

			// rest of the ELF identifier
			ident.clazz = ElfClass::BIT_64;
			ident.data = ElfData::LSB;
			ident.version = ELF_VERSION;
			ident.abi = 0;
			ident.abi_version = 0;
			memset(ident.pad, 0, sizeof(ident.pad));

			header.type = type;
			header.machine = machine;
			header.version = ELF_VERSION;
			header.entry = entrypoint;
			header.phoff = has_segments ? segment_headers->offset() : 0;
			header.shoff = has_sections ? section_headers->offset() : 0;
			header.flags = 0;
			header.ehsize = sizeof(header);
			header.phentsize = has_segments ? sizeof(ElfSegmentHeader) : 0;
			header.phnum = segment_headers->regions();
			header.shentsize = has_sections ? sizeof(ElfSectionHeader) : 0;
			header.shnum = section_headers->regions();
			header.shstrndx = has_sections ? 1 : 0;

		});
	}

	ElfFile::IndexedChunk ElfFile::section(const std::string& name, ElfSectionType type, const ElfSectionCreateInfo& info) {

		auto it = section_map.find(name);

		if (it != section_map.end()) {
			return it->second;
		}

		if (!has_sections) {
			define_section("", nullptr, ElfSectionType::NONE, {.alignment = 0});
			define_section(".shstrtab", section_string_table, ElfSectionType::STRTAB, {});
			has_sections = true;
		}

		auto region = info.segment == nullptr
			? sections->chunk(info.alignment)
			: info.segment->chunk(info.alignment);

		int index = define_section(name, region, type, info);
		section_map[name] = {region, index};

		return {region, index};
	}

	ElfFile::IndexedChunk ElfFile::segment(ElfSegmentType type, uint32_t flags, uint64_t address, uint64_t tail) {
		has_segments = true;
		const int alignment = getpagesize();
		auto region = segments->chunk(alignment, "segment");
		int index = define_segment(type, flags, region, address, tail, alignment);
		return {region, index};
	}

	void ElfFile::symbol(const std::string& name, ElfSymbolType type, ElfSymbolBinding binding, ElfSymbolVisibility visibility, int target, size_t offset, size_t size) {

		if (!has_symbols) {
			auto strings = section(".strtab", ElfSectionType::STRTAB, {});

			ElfSectionCreateInfo info {};
			info.link = [i = strings.index] noexcept { return i; };
			info.info = [this] noexcept { return local_symbols->bytes() / sizeof(ElfSymbol); };
			info.entry_size = sizeof(ElfSymbol);
			info.alignment = 8;

			auto symbols = section(".symtab", ElfSectionType::SYMTAB, info);

			symbol_strings = strings.data;
			local_symbols = symbols.data->chunk();
			other_symbols = symbols.data->chunk();
			has_symbols = true;

			// string table must start with \0
			symbol_strings->put<char>(0);

			// first symbol must be empty/undefined
			ElfSymbol symbol {};
			local_symbols->put<ElfSymbol>(symbol);
		}

		ElfSymbol symbol {};
		symbol.name = symbol_strings->bytes();
		symbol.type = type;
		symbol.binding = binding;
		symbol.visibility = visibility;
		symbol.shndx = target;
		symbol.value = offset;
		symbol.ssize = size;

		// local symbols must be first so we separate them here
		if (binding == ElfSymbolBinding::LOCAL) {
			local_symbols->put<ElfSymbol>(symbol);
		} else {
			other_symbols->put<ElfSymbol>(symbol);
		}

		symbol_strings->write(name);

	}

	std::shared_ptr<DwarfLineEmitter> ElfFile::line_emitter() {
		if (dwarf_line_emitter) {
			return dwarf_line_emitter;
		}

		auto chunk = section(".debug_line", ElfSectionType::PROGBITS, {});
		chunk.data->name = ".debug_line";

		dwarf_line_emitter = std::make_shared<DwarfLineEmitter>(chunk.data, 8);
		return dwarf_line_emitter;
	}

	std::shared_ptr<DwarfAbbreviations> ElfFile::dwarf_abbrev() {
		if (dwarf_abbrev_emitter) {
			return dwarf_abbrev_emitter;
		}

		auto chunk = section(".debug_abbrev", ElfSectionType::PROGBITS, {});
		chunk.data->name = ".debug_abbrev";

		dwarf_abbrev_emitter = std::make_shared<DwarfAbbreviations>(chunk.data);
		return dwarf_abbrev_emitter;
	}

	std::shared_ptr<DwarfInformation> ElfFile::dwarf_info() {
		if (dwarf_info_emitter) {
			return dwarf_info_emitter;
		}

		auto chunk = section(".debug_info", ElfSectionType::PROGBITS, {});
		chunk.data->name = ".debug_info";

		dwarf_info_emitter = std::make_shared<DwarfInformation>(chunk.data, dwarf_abbrev());
		return dwarf_info_emitter;
	}

	bool ElfFile::save(const std::string& path) const {

		using std::filesystem::perms;

		// if file creation fails return false
		try {
			auto buffer = bytes();
			std::ofstream output {path};

			if (output.bad()) {
				return false;
			}

			output.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
			output.close();
		} catch (const std::exception&) {
			return false;
		}

		// this part is best-effort only
		try {
			const perms flags = perms::owner_exec | perms::group_exec | perms::others_exec;
			std::filesystem::permissions(path, flags, std::filesystem::perm_options::add);
		} catch (const std::exception&) {}

		// file was created
		return true;
	}

	std::vector<uint8_t> ElfFile::bytes() const {
		return root->bake();
	}

	RunResult ElfFile::execute(const char* name) const {
		const char* argv[] = {name, nullptr};
		return execute(argv, (const char**) environ);
	}

	RunResult ElfFile::execute(const char** argv, const char** envp) const {
		// verify arguments, status can be a nullptr
		if (argv == nullptr || envp == nullptr) {
			return RunStatus::ARGS_ERROR;
		}

		// create in-memory file descriptor
		const int memfd = memfd_create("buffer", MFD_ALLOW_SEALING | MFD_CLOEXEC);
		if (memfd == -1) {
			return RunStatus::MEMFD_ERROR;
		}

		int* flag = (int*) mmap(nullptr, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if (flag == nullptr) {
			return RunStatus::MMAP_ERROR;
		}

		// copy buffer into memfd
		auto buffer = root->bake();
		write(memfd, buffer.data(), buffer.size());

		// we use this to check if the child really run or did execve just fail
		*flag = 0;

		// add seals to memfd
		if (fcntl(memfd, F_ADD_SEALS, F_SEAL_WRITE | F_SEAL_GROW | F_SEAL_SHRINK | F_SEAL_SEAL) != 0) {
			return RunStatus::SEAL_ERROR;
		}

		const pid_t pid = fork();
		if (pid == -1) {
			return RunStatus::FORK_ERROR;
		}

		// replace child with memfd elf file
		if (pid == 0) {
			fexecve(memfd, (char* const*) argv, (char* const*) envp);

			// if fexecve fails we need to kill ourselves
			*flag = 1;
			exit(1);
		}

		int status = 0;

		// wait for child and get status code
		if (waitpid(pid, &status, 0) == -1) {
			return RunStatus::WAIT_ERROR;
		}

		if (*flag) {
			return RunStatus::EXEC_ERROR;
		}

		munmap(flag, sizeof(int));

		// obtain return code from child status
		return WEXITSTATUS(status);
	}

}
