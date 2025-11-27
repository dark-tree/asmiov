
#define VSTL_TEST_COUNT 3
#define VSTL_PRINT_SKIP_REASON true
#define VSTL_SUBMODULE true

#include <vstl.hpp>

#include "util/tmp.hpp"
#include "out/elf/buffer.hpp"

namespace test::unit {

	using namespace asmio;

	inline std::string invoke(std::string cmd) {
		std::string out;
		char buf[256];

		cmd += " 2>&1";
		FILE* pipe = popen(cmd.c_str(), "r");
		if (!pipe) return out;

		while (fgets(buf, sizeof(buf), pipe)) {
			out += buf;
		}

		pclose(pipe);
		return out;
	}

	TEST(elf_segments) {

		ElfFile file {ElfMachine::X86_64, 0, 0};

		auto segment = file.segment(ElfSegmentType::LOAD, ElfSegmentFlags::R | ElfSegmentFlags::W, 0x42);
		segment.data->write("1234"); // 5 bytes

		util::TempFile temp {file};
		std::string result = invoke("readelf -a " + temp.path());

		ASSERT(!result.contains("Warning"));
		ASSERT(!result.contains("Error"));

		ASSERT(result.contains("LOAD           0x0000000000001000 0x0000000000000042 0x0000000000000000"));
		ASSERT(result.contains("               0x0000000000000005 0x0000000000000005  RW     0x1000"));

		ASSERT(result.contains("There are no section groups in this file."));
		ASSERT(result.contains("There are no sections in this file."));

	};

	TEST(elf_sections) {

		ElfFile file {ElfMachine::X86_64, 0, 0};

		file.section(".text", ElfSectionType::PROGBITS);
		file.section(".data", ElfSectionType::PROGBITS);

		util::TempFile temp {file};
		std::string result = invoke("readelf -a " + temp.path());

		ASSERT(!result.contains("Warning"));
		ASSERT(!result.contains("Error"));

		ASSERT(result.contains("[ 0]                   NULL"))
		ASSERT(result.contains("[ 1] .shstrtab         STRTAB"));
		ASSERT(result.contains("[ 2] .text             PROGBITS"));
		ASSERT(result.contains("[ 3] .data             PROGBITS"));

		ASSERT(result.contains("There are no section groups in this file."));
		ASSERT(result.contains("There are no program headers in this file."));

	};

	TEST(elf_symbols) {

		ElfFile file {ElfMachine::X86_64, 0, 0};

		auto executable = file.segment(ElfSegmentType::LOAD, ElfSegmentFlags::R | ElfSegmentFlags::X, 0);
		auto text = file.section(".text", ElfSectionType::PROGBITS, executable.data);

		file.symbol("iluvatar", ElfSymbolType::FUNC, ElfSymbolBinding::GLOBAL, ElfSymbolVisibility::DEFAULT, text, 0xca, 0);
		file.symbol("arda", ElfSymbolType::OBJECT, ElfSymbolBinding::LOCAL, ElfSymbolVisibility::DEFAULT, text, 0xdb, 1);
		file.symbol("melkor", ElfSymbolType::OBJECT, ElfSymbolBinding::GLOBAL, ElfSymbolVisibility::HIDDEN, text, 0xec, 44);

		util::TempFile temp {file};
		std::string result = invoke("readelf -a " + temp.path());

		ASSERT(!result.contains("Warning"));
		ASSERT(!result.contains("Error"));

		ASSERT(result.contains("0: 00000000000000db     1 OBJECT  LOCAL  DEFAULT    2 arda"));
		ASSERT(result.contains("1: 00000000000000ca     0 FUNC    GLOBAL DEFAULT    2 iluvatar"));
		ASSERT(result.contains("2: 00000000000000ec    44 OBJECT  GLOBAL HIDDEN     2 melkor"));
	};

}