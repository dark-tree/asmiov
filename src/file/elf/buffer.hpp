#pragma once

#include "external.hpp"
#include "util.hpp"
#include "header.hpp"
#include "section.hpp"
#include "segment.hpp"

namespace asmio::elf {

	enum struct RunResult : uint8_t {
		SUCCESS     = 0, // elf file was executed
		ARGS_ERROR  = 1, // the given arguments are invalid
		MEMFD_ERROR = 2, // memfd failed
		SEAL_ERROR  = 3, // fcntl failed
		FORK_ERROR  = 4, // fork failed
		EXEC_ERROR  = 5, // fexecve failed
		WAIT_ERROR  = 6, // waitpid failed
	};

	class ElfBuffer {

		private:

			std::vector<uint8_t> buffer;
			size_t data_length;

			size_t file_header_offset;
			size_t segment_header_offset;
			size_t segment_data_offset;

		public:

			/**
			 * creates a new ELF buffer
			 * @param length content size
			 * @param mount page aligned virtual mounting address
			 * @param entrypoint offset in bytes into the content where the execution will begin
			 */
			explicit ElfBuffer(size_t length, uint32_t mount, uint32_t entrypoint)
			: data_length(length) {
				file_header_offset = insert_struct<FileHeader>(buffer);
				segment_header_offset = insert_struct<SegmentHeader>(buffer);
				segment_data_offset = insert_struct<uint8_t>(buffer, length);

				FileHeader& header = *buffer_view<FileHeader>(buffer, file_header_offset);
				Identification& identifier = header.identifier;
				SegmentHeader& segment = *buffer_view<SegmentHeader>(buffer, segment_header_offset);

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
				header.machine = Machine::I386;
				header.version = VERSION;
				header.entrypoint = mount + segment_data_offset + entrypoint;
				header.flags = 0;
				header.header_size = sizeof(FileHeader);
				header.section_string_offset = UNDEFINED_SECTION;

				// ... segments
				header.program_table_offset = segment_header_offset;
				header.program_entry_size = sizeof(SegmentHeader);
				header.program_entry_count = 1;

				// ... sections
				header.section_table_offset = 0;
				header.section_entry_size = 0;
				header.section_entry_count = 0;

				// fill the segment header
				segment.type = SegmentType::LOAD;
				segment.virtual_address = mount;
				segment.physical_address = mount;
				segment.alignment = 0x1000;
				segment.file_size = buffer.size(); // load the entire ELF file to memory
				segment.memory_size = buffer.size();
				segment.offset = 0;
				segment.flags = SegmentFlags::R | SegmentFlags::W | SegmentFlags::X;

				// we can invalidate the struct pointers here safely
				buffer.shrink_to_fit();
			}

			void bake(const std::vector<uint8_t>& content) {
				if (content.size() != data_length) {
					throw std::runtime_error {"Invalid size!"};
				}

				memcpy(buffer_view<uint8_t>(buffer, segment_data_offset), content.data(), data_length);
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

				// copy buffer into memfd
				write(memfd, buffer.data(), size());

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
					return RunResult::EXEC_ERROR;
				}

				// wait for child and get status code
				if (waitpid(pid, status, 0) == -1) {
					return RunResult::WAIT_ERROR;
				}

				// obtain return code from child status
				*status = WEXITSTATUS(*status);
				return RunResult::SUCCESS;
			}

	};

}