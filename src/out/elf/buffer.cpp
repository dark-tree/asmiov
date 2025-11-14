
#include "buffer.hpp"

namespace asmio {

	/*
	 * class ElfFile
	 */

	void ElfFile::define_section(const std::string& name, const ChunkBuffer::Ptr& section, ElfSectionType type, uint32_t link, uint32_t info) {

		auto chunk = section_headers->chunk();

		chunk->link<ElfSectionHeader>([=, this] (auto& header) {

			header.name = section_string_table->bytes();
			header.type = type;
			header.flags = 0;
			header.addr = 0;

			if (section) {
				header.offset = section->offset();
				header.size = section->size();
			} else {
				header.offset = 0;
				header.size = 0;
			}

			header.link = link;
			header.info = info;
			header.addralign = 0;
			header.entsize = 0;

		});

		section_string_table->write(name);

	}

	void ElfFile::define_segment(ElfSegmentType type, uint32_t flags, const ChunkBuffer::Ptr& segment, uint64_t address, uint64_t tail, uint64_t align) {
		auto chunk = segment_headers->chunk();

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
	}

	ElfFile::ElfFile(ElfMachine machine, uint64_t mount, uint64_t entrypoint) {
		root = std::make_shared<ChunkBuffer>();

		auto chunk = root->chunk();

		section_headers = root->chunk();
		segment_headers = root->chunk();
		segments = root->chunk();
		sections = root->chunk();
		section_string_table = segments->chunk();

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

			header.type = ElfType::EXEC;
			header.machine = machine;
			header.version = ELF_VERSION;
			header.entry = mount + entrypoint;
			header.phoff = has_segments ? segment_headers->offset() : 0;
			header.shoff = has_sections ? section_headers->offset() : 0;
			header.flags = 0;
			header.ehsize = sizeof(header);
			header.phentsize = sizeof(ElfSegmentHeader);
			header.phnum = segment_headers->regions();
			header.shentsize = sizeof(ElfSectionHeader);
			header.shnum = section_headers->regions();
			header.shentsize = has_sections ? 1 : 0;

		});

	}

	ChunkBuffer::Ptr ElfFile::section(const std::string& name, ElfSectionType type, uint32_t link, uint32_t info, const ChunkBuffer::Ptr& segment) {

		if (!has_sections) {
			define_section("", nullptr, ElfSectionType::NONE, 0, 0);
			define_section(".shstrtab", section_string_table, ElfSectionType::STRTAB, 0, 0);
			has_sections = true;
		}

		auto region = segment == nullptr ? segments->chunk() : segment->chunk();
		define_section(name, region, type, link, info);
		return region;
	}

	ChunkBuffer::Ptr ElfFile::segment(ElfSegmentType type, uint32_t flags, uint64_t address, uint64_t tail) {
		has_segments = true;
		const int alignment = getpagesize();
		auto region = segments->chunk(alignment);
		define_segment(type, flags, region, address, tail, alignment);
		return region;
	}

	/*
	 * class ElfBuffer
	 */

	ElfBuffer::ElfBuffer(SegmentedBuffer& segmented, uint64_t mount, uint64_t entrypoint)
		: ElfFile(segmented.elf_machine, mount, entrypoint) {

		uint64_t address = mount;

		for (const BufferSegment& bs : segmented.segments()) {

			if (bs.empty()) {
				continue;
			}

			auto chunk = segment(ElfSegmentType::LOAD, to_elf_flags(bs), address, bs.tail);

			chunk->write(bs.buffer);
			chunk->push(bs.tail);

			address += bs.size();

		}

	}

	bool ElfBuffer::save(const char* path) const {
		auto buffer = root->bake();
		FILE* file = fopen(path, "wb");

		if (file == nullptr) {
			return false;
		}

		if (fwrite(buffer.data(), 1, buffer.size(), file) != buffer.size()) {
			return false;
		}

		fclose(file);
		return true;
	}

	RunResult ElfBuffer::execute(const char* name, int* status) const {
		const char* argv[] = {name, nullptr};
		return execute(argv, (const char**) environ, status);
	}

	RunResult ElfBuffer::execute(const char** argv, const char** envp, int* status) const {
		// verify arguments, status can be a nullptr
		if (argv == nullptr || envp == nullptr) {
			return RunResult::ARGS_ERROR;
		}

		// create in-memory file descriptor
		const int memfd = memfd_create("buffer", MFD_ALLOW_SEALING | MFD_CLOEXEC);
		if (memfd == -1) {
			return RunResult::MEMFD_ERROR;
		}

		int* flag = (int*) mmap(nullptr, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if (flag == nullptr) {
			return RunResult::MMAP_ERROR;
		}

		// copy buffer into memfd
		auto buffer = root->bake();
		write(memfd, buffer.data(), buffer.size());

		// we use this to check if the child really run or did execve just fail
		*flag = 0;

		// add seals to memfd
		if (fcntl(memfd, F_ADD_SEALS, F_SEAL_WRITE | F_SEAL_GROW | F_SEAL_SHRINK | F_SEAL_SEAL) != 0) {
			return RunResult::SEAL_ERROR;
		}

		const pid_t pid = fork();
		if (pid == -1) {
			return RunResult::FORK_ERROR;
		}

		// replace child with memfd elf file
		if (pid == 0) {
			fexecve(memfd, (char* const*) argv, (char* const*) envp);

			// if fexecve fails we need to kill ourselves
			*flag = 1;
			exit(1);
		}

		// wait for child and get status code
		if (waitpid(pid, status, 0) == -1) {
			return RunResult::WAIT_ERROR;
		}

		if (*flag) {
			return RunResult::EXEC_ERROR;
		}

		munmap(flag, sizeof(int));

		// obtain return code from child status
		*status = WEXITSTATUS(*status);
		return RunResult::SUCCESS;
	}

}