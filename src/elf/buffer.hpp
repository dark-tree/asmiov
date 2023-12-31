#pragma once

#include "external.hpp"
#include "util.hpp"

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <cstdio>

namespace asmio::elf {

	constexpr const uint32_t VERSION = 1;
	constexpr const uint32_t UNDEFINED_SECTION = 0;

	enum struct FileType : uint16_t {
		NONE = 0, // No file type
		REL  = 1, // Relocatable file
		EXEC = 2, // Executable file
		DYN  = 3, // Shared object file
		CORE = 4  // Core file
	};

	enum struct Machine : uint16_t {
		NONE  = 0, // No machine
		M32   = 1, // AT&T WE 32100
		SPARC = 2, // SPARC
		I386  = 3, // Intel 80386
		M68K  = 4, // Motorola 68000
		M88K  = 5, // Motorola 88000
		I860  = 7  // Intel 80860
	};

	enum struct Capacity : uint8_t {
		NONE   = 0, // Invalid class
		BIT_32 = 1, // 32-bit objects
		BIT_64 = 2  // 64-bit objects
	};

	enum struct Encoding : uint8_t {
		NONE = 0, // Invalid encoding
		LSB  = 1, // Two's complement, little-endian.
		MSB  = 2  // Two's complement, big-endian.
	};

	enum struct RunResult : uint8_t {
		SUCCESS     = 0, // elf file was executed
		ARGS_ERROR  = 1, // the given arguments are invalid
		MEMFD_ERROR = 2, // memfd failed
		SEAL_ERROR  = 3, // fcntl failed
		FORK_ERROR  = 4, // fork failed
		EXEC_ERROR  = 5, // fexecve failed
		WAIT_ERROR  = 6, // waitpid failed
	};

	enum struct SectionType : uint32_t {
		EMPTY    = 0,  // Inactive
		PROGBITS = 1,  // Information defined by the program, whose format and meaning are determined solely by the program
		SYMTAB   = 2,  // Symbol table
		STRTAB   = 3,  // String table
		RELA     = 4,  // Relocation entries with explicit addends
		HASH     = 5,  // Symbol hash table. All objects participating in dynamic linking must contain a symbol hash table.
		DYNAMIC  = 6,  // Information for dynamic linking
		NOTE     = 7,  // Custom attached metadata
		NOBITS   = 8,  // Occupies no space in the file but otherwise resembles SHT_PROGBITS.
		REL      = 9,  // Relocation entries without explicit addends
		SHLIB    = 10, // Reserved but has unspecified semantics
		DYNSYM   = 11, // Minimal set of dynamic linking symbols
	};

	struct SectionFlags {
		static constexpr const uint32_t WRITE = 0b001; // Writable section
		static constexpr const uint32_t ALLOC = 0b010; // Readable section
		static constexpr const uint32_t EXECI = 0b100; // Executable section
	};

	struct SegmentFlags { // why????
		static constexpr const uint32_t X = 0b001; // Executable segment
		static constexpr const uint32_t W = 0b010; // Writable segment
		static constexpr const uint32_t R = 0b100; // Readable segment
	};

	enum struct SegmentType : uint32_t {
		EMPTY   = 0, // Inactive
		LOAD    = 1, // Loadable segment
		DYNAMIC = 2, // Dynamic linking information
		INTERP  = 3, // Location and size of a null-terminated path name to invoke as an interpreter
		NOTE    = 4, // Custom attached metadata
		SHLIB   = 5, // Reserved but has unspecified semantics
		PHDR    = 6  // Specifies the location and size of the program header table itself
	};

	struct __attribute__((__packed__)) Identification {
		uint8_t magic[4];  // Magic number identifying the file as an ELF object file
		Capacity capacity; // Size of basic data types
		Encoding encoding; // Endianness
		uint8_t version;   // Same as FileHeader->version
		uint8_t pad[9];    // Must all be 0
	};

	struct __attribute__((__packed__)) FileHeader {
		Identification identifier;      // The initial bytes mark the file
		FileType type;                  // Specifies the file type
		Machine machine;                // Specifies the required architecture
		uint32_t version;               // Version of the Elf file, same as Identification->version
		uint32_t entrypoint;            // Virtual address to which the system first transfers control
		uint32_t program_table_offset;  // Offset to the program table header, or 0
		uint32_t section_table_offset;  // Offset to the section table header, or 0
		uint32_t flags;                 // Processor-specific flags associated with the file
		uint16_t header_size;           // Size of the FileHeader struct
		uint16_t program_entry_size;    // Size in bytes of one entry in the program table
		uint16_t program_entry_count;   // Number of entries in the program table
		uint16_t section_entry_size;    // Size in bytes of one entry in the section table
		uint16_t section_entry_count;   // Number of entries in the section table
		uint16_t section_string_offset; // Section index of the entry associated with the section name string table
	};

	struct __attribute__((__packed__)) RelocationInfo {
		uint32_t sym : 24;
		uint8_t type : 8;
	};

	struct __attribute__((__packed__)) ImplicitRelocation { // Elf32_Rel
		uint32_t offset;
		RelocationInfo info;
	};

	struct __attribute__((__packed__)) ExplicitRelocation { // Elf32_Rela
		uint32_t offset;
		RelocationInfo info;
		int32_t addend;
	};

	struct __attribute__((__packed__)) SectionHeader {
		uint32_t name;       // Offset into the string table
		SectionType type;    // Section type
		uint32_t flags;      // See SectionFlags
		uint32_t address;    // Virtual address at which the section should reside in memory
		uint32_t offset;     // Offset to the first byte of the section
		uint32_t size;       // section's size in bytes
		uint32_t link;       // Section header table index link
		uint32_t info;       // This member holds extra information
		uint32_t alignment;  // Alignment should be a positive, integral power of 2, and address must be congruent to 0, modulo the value of alignment
		uint32_t entry_size; // Size of one entry for sections where the entries have a fixed size, 0 otherwise
	};

	struct __attribute__((__packed__)) SegmentHeader {
		SegmentType type;          // Segment type
		uint32_t offset;           // Offset to the first byte of the segment
		uint32_t virtual_address;  // Virtual address at which the segment should reside in memory
		uint32_t physical_address; // On systems for which physical addressing is relevant, physical address at which the segment should reside in memory
		uint32_t file_size;        // Number of bytes in the file image of the segment
		uint32_t memory_size;      // Number of bytes in the memory image of the segment
		uint32_t flags;            // See SegmentFlags
		uint32_t alignment;        // Alignment should be a positive, integral power of 2, and virtual_address should equal offset, modulo alignment
	};

	class ElfBuffer {

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
				segment.file_size = buffer.size(); // we load the entire ELF file to memory
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

			RunResult execve(const char** argv, const char** envp, int* status) {
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
				write(memfd, data(), size());

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

				// wait for child and get return code
				if (waitpid(pid, status, 0) == -1) {
					return RunResult::WAIT_ERROR;
				}

				return RunResult::SUCCESS;
			}

	};

}