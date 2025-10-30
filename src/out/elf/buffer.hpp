#pragma once

#include "external.hpp"
#include "util.hpp"
#include "header.hpp"
#include "section.hpp"
#include "segment.hpp"
#include "out/buffer/segmented.hpp"
#include <sys/stat.h>

#define DEFAULT_ELF_MOUNT 0x08048000

namespace asmio::elf {

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

	class ElfBuffer {

		private:

			std::vector<uint8_t> buffer;
			size_t data_length;

			size_t file_header_offset;
			size_t segment_header_offset;
			size_t segment_data_offset;

			uint32_t translate_flags(const BufferSegment& segment) {
				uint32_t flags = 0;
				if (segment.flags & BufferSegment::R) flags |= SegmentFlags::R;
				if (segment.flags & BufferSegment::W) flags |= SegmentFlags::W;
				if (segment.flags & BufferSegment::X) flags |= SegmentFlags::X;

				return flags;
			}

		public:

			/**
			 * creates a new ELF buffer
			 * @param segmented content
			 * @param mount page aligned virtual mounting address
			 * @param entrypoint offset in bytes into the content where the execution will begin
			 */
			explicit ElfBuffer(SegmentedBuffer& segmented, uint64_t mount, uint64_t entrypoint)
			: data_length(segmented.total()) {
				file_header_offset = util::insert_struct<FileHeader>(buffer);
				segment_header_offset = util::insert_struct<SegmentHeader>(buffer, segmented.count());

				// calculate number of bytes to the next page boundry
				const size_t page = getpagesize();
				const size_t size = buffer.size();
				const size_t tail = ALIGN_UP(size, page) - size;

				// align to page boundary
				util::insert_struct<uint8_t>(buffer, tail);

				// page aligned data buffer
				segment_data_offset = util::insert_struct<uint8_t>(buffer, data_length);

				FileHeader& header = *util::buffer_view<FileHeader>(buffer, file_header_offset);
				Identification& identifier = header.identifier;

				// ELF magic number
				identifier.magic[0] = 0x7f;
				identifier.magic[1] = 'E';
				identifier.magic[2] = 'L';
				identifier.magic[3] = 'F';

				// rest of the ELF identifier
				identifier.capacity = Capacity::BIT_32;
				identifier.encoding = Encoding::LSB;
				identifier.version = VERSION;
				memset(identifier.pad, 0, 9);

				// fill the file header
				header.type = FileType::EXEC;
				header.machine = Machine::X86_64;
				header.version = VERSION;
				header.entrypoint = mount + entrypoint;
				header.flags = 0;
				header.header_size = sizeof(FileHeader);
				header.section_string_offset = UNDEFINED_SECTION;

				// ... segments
				header.program_table_offset = segment_header_offset;
				header.program_entry_size = sizeof(SegmentHeader);
				header.program_entry_count = 0;

				// ... sections
				header.section_table_offset = 0;
				header.section_entry_size = 0;
				header.section_entry_count = 0;

				uint64_t header_offset = segment_header_offset;
				uint64_t data_offset = segment_data_offset;
				uint64_t address = mount;

				for (const BufferSegment& bs : segmented.segments()) {
					SegmentHeader& segment = *util::buffer_view<SegmentHeader>(buffer, header_offset);

					if (bs.empty()) {
						continue;
					}

					// fill the segment header
					segment.type = SegmentType::LOAD;
					segment.virtual_address = address;
					segment.physical_address = address;
					segment.alignment = 0x1000;
					segment.file_size = bs.size();
					segment.memory_size = bs.size();
					segment.offset = data_offset;
					segment.flags = translate_flags(bs);

					// store the data
					memcpy(util::buffer_view<uint8_t>(buffer, data_offset), bs.buffer.data(), bs.buffer.size());
					// TODO the trailing tail padding is ignored here and left unset

					// move the window forward
					data_offset += segment.file_size;
					header_offset += sizeof(SegmentHeader);
					address += segment.file_size;
					header.program_entry_count ++;
				}

				// we can invalidate the struct pointers here safely
				buffer.shrink_to_fit();
			}

		public:

			/// returns a pointer to a buffer that contains the file
			uint8_t* data() {
				return buffer.data();
			}

			/// returns the file size in bytes
			size_t size() const {
				return buffer.size();
			}

			/// returns the offset from the mount address to the fist byte of content
			size_t offset() const {
				return segment_data_offset;
			}

			bool save(const char* path) const {
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
				write(memfd, buffer.data(), size());

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