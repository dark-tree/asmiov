#pragma once

#include <ranges>

#include "external.hpp"
#include "util.hpp"
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

			void define_section(const std::string& name, const ChunkBuffer::Ptr& section, ElfSectionType type, uint32_t link, uint32_t info) {

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

			void define_segment(ElfSegmentType type, uint32_t flags, const ChunkBuffer::Ptr& segment, uint64_t address, uint64_t tail, uint64_t align) {
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

		public:

			ElfFile(ElfMachine machine, uint64_t mount, uint64_t entrypoint) {
				root = std::make_shared<ChunkBuffer>();

				auto header = root->chunk();

				section_headers = root->chunk();
				segment_headers = root->chunk();
				segments = root->chunk();
				sections = root->chunk();
				section_string_table = segments->chunk();

				header->link<ElfFileHeader>([=, this] (auto& header) {

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

			ChunkBuffer::Ptr section(const std::string& name, ElfSectionType type, uint32_t link, uint32_t info, const ChunkBuffer::Ptr& segment = nullptr) {

				if (!has_sections) {
					define_section("", nullptr, ElfSectionType::NONE, 0, 0);
					define_section(".shstrtab", section_string_table, ElfSectionType::STRTAB, 0, 0);
					has_sections = true;
				}

				auto region = segment == nullptr ? segments->chunk() : segment->chunk();
				define_section(name, region, type, link, info);
				return region;
			}

			ChunkBuffer::Ptr segment(ElfSegmentType type, uint32_t flags, uint64_t address, uint64_t tail = 0) {
				has_segments = true;
				const int alignment = getpagesize();
				auto region = segments->chunk(alignment);
				define_segment(type, flags, region, address, tail, alignment);
				return region;
			}

	};

	class ElfBuffer : ElfFile {

		public:

			/**
			 * creates a new ELF buffer
			 * @param segmented content
			 * @param mount page aligned virtual mounting address
			 * @param entrypoint offset in bytes into the content where the execution will begin
			 */
			explicit ElfBuffer(SegmentedBuffer& segmented, uint64_t mount, uint64_t entrypoint)
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

		public:

			bool save(const char* path) const {
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

			RunResult execute(const char* name, int* status) const {
				const char* argv[] = {name, nullptr};
				return execute(argv, (const char**) environ, status);
			}

			RunResult execute(const char** argv, const char** envp, int* status) const {
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