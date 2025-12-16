
#define VSTL_TEST_COUNT 3
#define VSTL_PRINT_SKIP_REASON true
#define VSTL_SUBMODULE true

#include <vstl.hpp>
#include <out/buffer/writer.hpp>
#include <out/elf/dwarf/encoding.hpp>
#include <tasml/top.hpp>

#include "util/tmp.hpp"
#include "out/elf/export.hpp"
#include "test.hpp"

namespace test {

	using namespace asmio;

	TEST(elf_segments) {

		ElfFile file {ElfMachine::X86_64, ElfType::EXEC, 0};

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

		ElfFile file {ElfMachine::X86_64, ElfType::EXEC, 0};

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

		ElfFile file {ElfMachine::X86_64, ElfType::EXEC, 0};

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

	TEST(elf_line_section) {

		ElfFile file {ElfMachine::X86_64, ElfType::EXEC, 0};

		auto emitter1 = file.line_emitter();
		auto emitter2 = file.line_emitter();

		CHECK(emitter1, emitter2);

		DwarfDir d = emitter2->add_directory("./");
		DwarfFile f = emitter2->add_file(d, "hate.txt");
		emitter2->set_mapping(0xFFFFFF, f, 1, 1);
		emitter2->end_sequence();

		util::TempFile temp {file};
		std::string result = call_shell("readelf -aw " + temp.path());

		ASSERT(!result.contains("Warning"));
		ASSERT(!result.contains("Error"));

		ASSERT(result.contains("[ 2] .debug_line       PROGBITS"));

		ASSERT(result.contains("Line Base:                   -3"));
		ASSERT(result.contains("Line Range:                  12"));
		ASSERT(result.contains("Opcode Base:                 13"));

		ASSERT(result.contains("0	./"));
		ASSERT(result.contains("0	0	hate.txt"));

		ASSERT(result.contains("[0x00000035]  Set File Name to entry 0 in the File Name Table"));
		ASSERT(result.contains("[0x00000037]  Set column to 1"));
		ASSERT(result.contains("[0x00000039]  Advance PC by 16777215 to 0xffffff"));
		ASSERT(result.contains("[0x0000003f]  Extended opcode 1: End of Sequence"));

	};

	TEST(elf_line_sequence) {

		ElfFile file {ElfMachine::X86_64, ElfType::EXEC, 0};

		auto emitter = file.line_emitter();

		DwarfDir d = emitter->add_directory("./");
		DwarfFile f1 = emitter->add_file(d, "caine.txt");
		DwarfFile f2 = emitter->add_file(d, "abel.txt");

		emitter->set_mapping(0x20000000, f1, 1, 1);
		emitter->set_mapping(0x20000004, f1, 2, 1);
		emitter->set_mapping(0x20000005, f1, 42, 1);
		emitter->set_mapping(0x20000006, f1, 42, 3);

		emitter->set_mapping(0x10000000, f2, 1, 1);
		emitter->set_mapping(0x10000014, f2, 2, 1); // test special case
		emitter->end_sequence();

		util::TempFile temp {file};
		std::string result = call_shell("readelf -aw " + temp.path());

		ASSERT(!result.contains("Warning"));
		ASSERT(!result.contains("Error"));

		auto line_block = util::split_string(result, "Line Number Statements:").at(1);
		auto lines = util::normalize_strings(util::split_string(line_block));

		std::vector<std::string> expected = {
			"[0x00000040]  Set File Name to entry 0 in the File Name Table",
			"[0x00000042]  Set column to 1",
			"[0x00000044]  Advance PC by 536870912 to 0x20000000",
			"[0x0000004a]  Copy",
			"[0x0000004b]  Special opcode 52: advance Address by 4 to 0x20000004 and Line by 1 to 2",
			"[0x0000004c]  Advance Line by 40 to 42",
			"[0x0000004e]  Special opcode 15: advance Address by 1 to 0x20000005 and Line by 0 to 42",
			"[0x0000004f]  Set column to 3",
			"[0x00000051]  Special opcode 15: advance Address by 1 to 0x20000006 and Line by 0 to 42",
			"[0x00000052]  Set File Name to entry 1 in the File Name Table",
			"[0x00000054]  Extended opcode 2: set Address to 0x10000000",
			"[0x0000005f]  Set column to 1",
			"[0x00000061]  Advance Line by -41 to 1",
			"[0x00000063]  Copy",
			"[0x00000064]  Advance PC by 20 to 0x10000014",
			"[0x00000066]  Advance Line by 1 to 2",
			"[0x00000068]  Copy",
			"[0x00000069]  Extended opcode 1: End of Sequence",
		};

		CHECK(lines, expected);

	};

	TEST (elf_tasml_source_mapping) {

		std::string code = R"(
			source "./test/foo.bar" 21 37
			byte 1
			source "./test/foo.bar" 22 37
			byte 2
		)";

		tasml::ErrorHandler reporter {vstl_self.name, true};
		auto program = tasml::assemble(reporter, code);
		ASSERT(reporter.ok());

		ElfFile file = to_elf(program, Label::UNSET);
		util::TempFile object {file, ".o"};

		std::string result = call_shell("readelf -aw " + object.path());

		ASSERT(!result.contains("Warning"));
		ASSERT(!result.contains("Error"));

		auto line_block = util::split_string(result, "Line Number Statements:").at(1);
		auto lines = util::normalize_strings(util::split_string(line_block));

		std::vector<std::string> expected = {
			"[0x0000003a]  Set File Name to entry 0 in the File Name Table",
			"[0x0000003c]  Set column to 37",
			"[0x0000003e]  Advance Line by 20 to 21",
			"[0x00000040]  Copy",
			"[0x00000041]  Special opcode 16: advance Address by 1 to 0x1 and Line by 1 to 22",
			"[0x00000042]  Extended opcode 1: End of Sequence"
		};

		CHECK(lines, expected);

	};

	TEST (elf_dwarf_abbreviations) {

		ElfFile file {ElfMachine::X86_64, ElfType::EXEC, 0};
		auto emitter = file.dwarf_abbrev();

		auto t1 = DwarfObjectBuilder::of(DwarfTag::base_type)
			.add(DwarfAttr::name, DwarfForm::string)
			.add(DwarfAttr::encoding, DwarfForm::data1)
			.add(DwarfAttr::byte_size, DwarfForm::data1);

		auto t2 = DwarfObjectBuilder::of(DwarfTag::pointer_type)
			.add(DwarfAttr::type, DwarfForm::ref4);

		// same as t1
		auto t3 = DwarfObjectBuilder::of(DwarfTag::base_type)
			.add(DwarfAttr::name, DwarfForm::string)
			.add(DwarfAttr::encoding, DwarfForm::data1)
			.add(DwarfAttr::byte_size, DwarfForm::data1);

		int a = emitter->submit(t1);
		int b = emitter->submit(t1); // excluded
		int c = emitter->submit(t2);
		int d = emitter->submit(t3); // excluded

		CHECK(a, 1);
		CHECK(b, 1);
		CHECK(c, 2);
		CHECK(d, 1);

		util::TempFile object {file};
		std::string result = call_shell("readelf -aw " + object.path());

		ASSERT(!result.contains("Warning"));
		ASSERT(!result.contains("Error"));

		ASSERT(result.contains("1      DW_TAG_base_type    [no children]"));
		ASSERT(result.contains("2      DW_TAG_pointer_type    [no children]"));
		ASSERT(result.contains("DW_AT_name         DW_FORM_string"));
		ASSERT(result.contains("DW_AT_encoding     DW_FORM_data1"));
		ASSERT(result.contains("DW_AT_byte_size    DW_FORM_data1"));
		ASSERT(result.contains("DW_AT_type         DW_FORM_ref4"));
		ASSERT(result.contains("DW_AT value: 0     DW_FORM value: 0"));

	};

	TEST (elf_dwarf_info) {

		ElfFile file {ElfMachine::X86_64, ElfType::EXEC, 0};
		auto emitter = file.dwarf_info();

		auto t1 = DwarfObjectBuilder::of(DwarfTag::base_type)
			.add(DwarfAttr::name, DwarfForm::string)
			.add(DwarfAttr::encoding, DwarfForm::data1)
			.add(DwarfAttr::byte_size, DwarfForm::data1);

		auto t2 = DwarfObjectBuilder::of(DwarfTag::pointer_type)
			.add(DwarfAttr::type, DwarfForm::ref4);

		auto unit = emitter->compile_unit("my_unit.s");

		auto s1 = emitter->submit(t1, unit);
		s1->write("test_t");
		s1->put<uint8_t>(DwarfEncoding::UNSIGNED);
		s1->put<uint8_t>(4);

		auto s2 = emitter->submit(t2, unit);
		s2->put<uint32_t>(s1.offset);

		util::TempFile object {file};
		std::string result = call_shell("readelf -aw " + object.path());

		ASSERT(!result.contains("Warning"));
		ASSERT(!result.contains("Error"));

		ASSERT(result.contains("Abbrev Number: 1 (DW_TAG_compile_unit)"));
		ASSERT(result.contains("Abbrev Number: 2 (DW_TAG_base_type)"));
		ASSERT(result.contains("Abbrev Number: 3 (DW_TAG_pointer_type)"));
		ASSERT(result.contains("DW_AT_name        : test_t"));
		ASSERT(result.contains("DW_AT_name        : my_unit.s"));
		ASSERT(result.contains("DW_AT_encoding    : 7	(unsigned)"));
		ASSERT(result.contains("DW_AT_type        : <0x24>"));

	};

}