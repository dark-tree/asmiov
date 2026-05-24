#pragma once

#include <asmio/external.hpp>
#include <asmio/util/macro.hpp>

namespace asmio {

	constexpr uint32_t VERSION = 1;

	enum struct ElfType : uint16_t {
		NONE = 0,  ///< No file type
		REL  = 1,  ///< Relocatable file
		EXEC = 2,  ///< Executable file
		DYN  = 3,  ///< Shared object file
		CORE = 4,  ///< Core file
	};

	enum struct ElfMachine : uint16_t {
		NONE    = 0,   ///< No machine
		I386    = 3,   ///< Intel 80386
		I860    = 7,   ///< Intel 80860
		I960    = 19,  ///< Intel 80960
		X86_64  = 62,  ///< AMD x86-64 architecture
		ARM     = 40,  ///< ARM
		AARCH64 = 183, ///< AArch64
		RISCV   = 243, ///< RISC-V
		AVR     = 83,  ///< Atmel AVR 8-bit microcontroller

#if ARCH_AARCH64
		NATIVE = AARCH64,
#endif

#if ARCH_X86
		NATIVE = X86_64,
#endif

#if ARCH_RISCV64
		NATIVE = RISCV,
#endif
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
	
	enum struct ElfAbi : uint8_t {
		NONE       = 0,   ///< UNIX System V ABI
		SYSV       = 0,   ///< Alias.
		HPUX       = 1,   ///< HP-UX
		NETBSD     = 2,   ///< NetBSD.
		GNU        = 3,   ///< Object uses GNU ELF extensions.
		LINUX      = 3,   ///< Compatibility alias.
		SOLARIS    = 6,   ///< Sun Solaris.
		AIX        = 7,   ///< IBM AIX.
		IRIX       = 8,   ///< SGI Irix.
		FREEBSD    = 9,   ///< FreeBSD.
		TRU64      = 10,  ///< Compaq TRU64 UNIX.
		MODESTO    = 11,  ///< Novell Modesto.
		OPENBSD    = 12,  ///< OpenBSD.
		ARM_AEABI  = 64,  ///< ARM EABI
		ARM        = 97,  ///< ARM
		STANDALONE = 255, ///< Standalone (embedded) application
	};

	struct PACKED ElfIdentification {
		uint8_t magic[4];    ///< Magic number identifying the file as an ELF object file
		ElfClass clazz;      ///< Size of basic data types
		ElfData data;        ///< Endianness
		uint8_t version;     ///< Same as FileHeader->version
		ElfAbi abi;         ///< Target operating system
		uint8_t abi_version; ///< Version of the ABI to which the object is targeted
		uint8_t pad[7];      ///< Must all be 0
	};

	struct PACKED ElfFileHeader {
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