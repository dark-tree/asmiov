#pragma once

#include "external.hpp"

namespace asmio {

	constexpr uint32_t VERSION = 1;

	enum struct ElfType : uint16_t {
		NONE = 0,  // No file type
		REL  = 1,  // Relocatable file
		EXEC = 2,  // Executable file
		DYN  = 3,  // Shared object file
		CORE = 4,  // Core file
	};

	enum struct ElfMachine : uint16_t {
		NONE  = 0,  // No machine
		M32   = 1,  // AT&T WE 32100
		SPARC = 2,  // SPARC
		I386  = 3,  // Intel 80386
		M68K  = 4,  // Motorola 68000
		M88K  = 5,  // Motorola 88000
		I860  = 7,  // Intel 80860
		X86_64 = 62, // AMD x86-64 architecture
		AARCH64 = 183 // AArch64
	};

	enum struct ElfClass : uint8_t {
		NONE   = 0, // Invalid class
		BIT_32 = 1, // 32-bit objects
		BIT_64 = 2, // 64-bit objects
	};

	enum struct ElfData : uint8_t {
		NONE = 0,   // Invalid encoding
		LSB  = 1,   // Two's complement, little-endian.
		MSB  = 2,   // Two's complement, big-endian.
	};

	struct __attribute__((__packed__)) ElfIdentification {
		uint8_t magic[4];    ///< Magic number identifying the file as an ELF object file
		ElfClass clazz;      ///< Size of basic data types
		ElfData data;        ///< Endianness
		uint8_t version;     ///< Same as FileHeader->version
		uint8_t abi;         ///< Target operating system
		uint8_t abi_version; ///< Version of the ABI to which the object is targeted
		uint8_t pad[7];      ///< Must all be 0
	};

	struct __attribute__((__packed__)) ElfFileHeader {
		ElfIdentification identification; ///< The initial bytes mark the file
		ElfType type;        ///< Specifies the file type
		ElfMachine machine;  ///< Specifies the required architecture
		uint32_t version;    ///< Version of the Elf file, same as Identification->version
		uint64_t entry;      ///< Virtual address to which the system first transfers control
		uint64_t phoff;      ///< Offset to the program table header, or 0
		uint64_t shoff;      ///< Offset to the section table header, or 0
		uint32_t flags;      ///< Processor-specific flags associated with the file
		uint16_t ehsize;     ///< Size of the FileHeader struct
		uint16_t phentsize;  ///< Size in bytes of one entry in the program table
		uint16_t phnum;      ///< Number of entries in the program table
		uint16_t shentsize;  ///< Size in bytes of one entry in the section table
		uint16_t shnum;      ///< Number of entries in the section table
		uint16_t shstrndx;   ///< Section index of the entry associated with the section name string table
	};

}