#pragma once

#include <external.hpp>
#include <macro.hpp>

enum struct ElfSymbolVisibility : uint8_t {
	DEFAULT   = 0, ///< Visible to other modules, local references can use symbols from other modules
	HIDDEN    = 2, ///< Hidden to other modules, local references always use local symbol
	PROTECTED = 3, ///< Visible to other modules, local references always use local symbol
};

enum struct ElfSymbolType : uint8_t {
	NOTYPE    = 0, ///< No defined type
	OBJECT    = 1, ///< Data object
	FUNC      = 2, ///< Code code
	SECTION   = 3, ///< Symbol associated with a section
	FILE      = 4, ///< Symbol's name is file name
	COMMON    = 5, ///< Common data object
	TLS       = 6, ///< Thread-local data object
};

enum struct ElfSymbolBinding : uint8_t {
	LOCAL     = 0, ///< Not visible outside the same object file
	GLOBAL    = 1, ///< Visible in other objects being linked
	WEAK      = 2, ///< Global symbol of lower precedence
};

struct PACKED ElfSymbol {
	uint32_t name = 0;              ///< Offset into the string section
	ElfSymbolType type : 4;         ///< Symbol's type
	ElfSymbolBinding binding : 4;   ///< Symbol's binding
	ElfSymbolVisibility visibility; ///< Symbol's visibility
	uint16_t shndx = 0;             ///< Symbol's section
	uint64_t value = 0;             ///< Value (e.g. address, offset) of a symbol
	uint64_t ssize = 0;             ///< Size of the symbol, or zero if not applicable
};