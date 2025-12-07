
#define VSTL_TEST_COUNT 3
#define VSTL_PRINT_SKIP_REASON true
#define VSTL_SUBMODULE true

#include <vstl.hpp>
#include <out/buffer/writer.hpp>
#include <tasml/top.hpp>

#include "util/tmp.hpp"
#include "out/elf/buffer.hpp"
#include "test.hpp"

namespace test {

	using namespace asmio;

	TEST(elf_segments) {

		ElfFile file {ElfMachine::X86_64, ElfType::EXEC, 0, 0};

		auto segment = file.segment(ElfSegmentType::LOAD, ElfSegmentFlags::R | ElfSegmentFlags::W, 0x42);
		segment.data->write("1234"); // 5 bytes

		util::TempFile temp {file};
		std::string result = call_shell("readelf -a " + temp.path());

		ASSERT(!result.contains("Warning"));
		ASSERT(!result.contains("Error"));

		ASSERT(result.contains("ELF64"));
		ASSERT(result.contains("Advanced Micro Devices X86-64"));

		ASSERT(result.contains("LOAD           0x0000000000001000 0x0000000000000042 0x0000000000000000"));
		ASSERT(result.contains("               0x0000000000000005 0x0000000000000005  RW     0x1000"));

		ASSERT(result.contains("There are no section groups in this file."));
		ASSERT(result.contains("There are no sections in this file."));

	};

	TEST(elf_sections) {

		ElfFile file {ElfMachine::X86_64, ElfType::EXEC, 0, 0};

		file.section(".text", ElfSectionType::PROGBITS, {});
		file.section(".data", ElfSectionType::PROGBITS, {});

		util::TempFile temp {file};
		std::string result = call_shell("readelf -a " + temp.path());

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

		ElfFile file {ElfMachine::X86_64, ElfType::EXEC, 0, 0};

		auto executable = file.segment(ElfSegmentType::LOAD, ElfSegmentFlags::R | ElfSegmentFlags::X, 0);
		auto text = file.section(".text", ElfSectionType::PROGBITS, { .segment = executable.data }).index;

		file.symbol("iluvatar", ElfSymbolType::FUNC, ElfSymbolBinding::GLOBAL, ElfSymbolVisibility::DEFAULT, text, 0xca, 0);
		file.symbol("arda", ElfSymbolType::OBJECT, ElfSymbolBinding::LOCAL, ElfSymbolVisibility::DEFAULT, text, 0xdb, 1);
		file.symbol("melkor", ElfSymbolType::OBJECT, ElfSymbolBinding::GLOBAL, ElfSymbolVisibility::HIDDEN, text, 0xec, 44);

		util::TempFile temp {file};
		std::string result = call_shell("readelf -a " + temp.path());

		ASSERT(!result.contains("Warning"));
		ASSERT(!result.contains("Error"));

		ASSERT(result.contains("0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND"));
		ASSERT(result.contains("1: 00000000000000db     1 OBJECT  LOCAL  DEFAULT    2 arda"));
		ASSERT(result.contains("2: 00000000000000ca     0 FUNC    GLOBAL DEFAULT    2 iluvatar"));
		ASSERT(result.contains("3: 00000000000000ec    44 OBJECT  GLOBAL HIDDEN     2 melkor"));
	};

	TEST(elf_writer_no_exported_symbol) {

		SegmentedBuffer buffer;
		BasicBufferWriter writer {buffer};

		writer.label("aaaa");
		writer.put_dword(0xAAAAAAAA);

		writer.label("bbbb");
		writer.put_dword(0xBBBBBBBB);

		writer.label("cccc");
		writer.put_dword(0xCCCCCCCC);

		writer.label("dddd");
		writer.put_dword(0xDDDDDDDD);

		ElfFile file = to_elf(buffer, "dddd");
		util::TempFile temp {file};

		std::string result = call_shell("readelf -a " + temp.path());

		ASSERT(!result.contains("Warning"));
		ASSERT(!result.contains("Error"));

		// TODO support omitting section info
		// ASSERT(result.contains("There are no section groups in this file."));
		// ASSERT(result.contains("There are no sections in this file."));

		SKIP("Unimplemented feature");

	}

	TEST(elf_writer_exported_symbol) {

		SegmentedBuffer buffer;
		BasicBufferWriter writer {buffer};

		writer.section(MemoryFlag::R);
		writer.export_symbol("aaaa");
		writer.export_symbol("bbbb");
		writer.export_symbol("cccc");
		writer.export_symbol("dddd");

		writer.label("aaaa");
		writer.put_dword(0xAAAAAAAA);

		writer.label("bbbb");
		writer.put_dword(0xBBBBBBBB);

		writer.label("cccc");
		writer.put_dword(0xCCCCCCCC);

		writer.label("dddd");
		writer.put_dword(0xDDDDDDDD);

		ElfFile file = to_elf(buffer, "dddd");
		util::TempFile temp {file};

		std::string result = call_shell("readelf -a " + temp.path());

		ASSERT(!result.contains("Warning"));
		ASSERT(!result.contains("Error"));

		ASSERT(result.contains("0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND"));
		ASSERT(result.contains("1: 0000000000000000     0 OBJECT  GLOBAL PROTECTED    2 aaaa"));
		ASSERT(result.contains("2: 0000000000000004     0 OBJECT  GLOBAL PROTECTED    2 bbbb"));
		ASSERT(result.contains("3: 0000000000000008     0 OBJECT  GLOBAL PROTECTED    2 cccc"));
		ASSERT(result.contains("4: 000000000000000c     0 OBJECT  GLOBAL PROTECTED    2 dddd"));

	};

	TEST(elf_auto_sections) {

		SegmentedBuffer buffer;
		BasicBufferWriter writer {buffer};

		writer.label("abc");
		writer.put_dword(0);
		writer.section(MemoryFlag::R);
		writer.put_dword(0);
		writer.section(MemoryFlag::R | MemoryFlag::X);
		writer.put_dword(0);
		writer.section(MemoryFlag::R);
		writer.put_dword(0);
		writer.section(MemoryFlag::R, ".custom");
		writer.put_dword(0);
		writer.section(MemoryFlag::R | MemoryFlag::W);
		writer.put_dword(0);

		ElfFile file = to_elf(buffer, "abc");
		util::TempFile temp {file};

		std::string result = call_shell("readelf -a " + temp.path());

		ASSERT(!result.contains("Warning"));
		ASSERT(!result.contains("Error"));

		ASSERT(result.contains("[ 2] .rwx              PROGBITS"));
		ASSERT(result.contains("[ 3] .rodata           PROGBITS"));
		ASSERT(result.contains("[ 4] .text             PROGBITS"));
		ASSERT(result.contains("[ 5] .custom           PROGBITS"));
		ASSERT(result.contains("[ 6] .data             PROGBITS"));

	};

	TEST(elf_gcc_linker_compatible) {

		std::string code = R"(
			section r
			export test: byte "hello!", 0
		)";

		tasml::ErrorHandler reporter {vstl_self.name, true};
		SegmentedBuffer buffer = tasml::assemble(reporter, code);

		if (!reporter.ok()) {
			reporter.dump();
			FAIL("Errors generated");
		}

		// override the architecture
		buffer.elf_machine = ElfMachine::NATIVE;

		ElfFile file = to_elf(buffer, Label::UNSET);
		util::TempFile object {file, ".tasml.o"};

		std::string result = call_shell("readelf -a " + object.path());

		ASSERT(!result.contains("Warning"));
		ASSERT(!result.contains("Error"));

		ASSERT(result.contains("REL (Relocatable file)"));
		ASSERT(result.contains("0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND"));
		ASSERT(result.contains("1: 0000000000000000     0 OBJECT  GLOBAL PROTECTED    2 test"));

		util::TempFile main_src {".main.c"};
		main_src.write(R"(
			#include <stdio.h>

			extern const char test[];

			int main() {
				printf("%s", test);
			}
		)");

		// link with our object
		util::TempFile exec {".out"};
		std::string gcc_output = call_shell("gcc -z noexecstack -o " + exec.path() + " " + object.path() + " " + main_src.path() );
		CHECK(gcc_output, "");

		std::string exe_output = call_shell(exec.path());
		CHECK(exe_output, "hello!");

	};

}