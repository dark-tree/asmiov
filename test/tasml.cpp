
#define DEBUG_MODE false
#define VSTL_TEST_COUNT 3
#define VSTL_PRINT_SKIP_REASON true
#define VSTL_SUBMODULE true

#include <filesystem>
#include <fstream>
#include <regex>
#include <tasml/top.hpp>
#include <util/tmp.hpp>
#include <out/elf/export.hpp>

#include "test.hpp"
#include "vstl.hpp"
#include "out/buffer/executable.hpp"

namespace test {

	using namespace asmio;

	TEST (tasml_check_basic_error) {

		std::string code = R"(
			lang aarch64
			b 7, @test
		)";

		tasml::ErrorHandler reporter {vstl_self.name(), true};

		EXPECT_THROW(std::runtime_error) {
			tasml::assemble(reporter, code);
		};

		ASSERT(!reporter.ok());

	};

	TEST (tasml_check_overloaded_mnemonics) {

		std::string code = R"(
			lang aarch64
			mov x1, 7
			mov x2, x1
		)";

		tasml::ErrorHandler reporter {vstl_self.name(), true};
		tasml::assemble(reporter, code);

		ASSERT(reporter.ok());

	};

	TEST (tasml_emit_x86) {

		std::string code = R"(
			lang x86
			section rx

			// random instructions
			label:
			mov rcx, rax
			dec rax
			cmp byte [rax + rbx * 2], 0
			jne @label
			nop
		)";

		tasml::ErrorHandler reporter {vstl_self.name(), true};
		SegmentedBuffer buffer = tasml::assemble(reporter, code);

		if (!reporter.ok()) {
			reporter.dump();
			FAIL("Errors generated");
		}

	};

	TEST (tasml_emit_aarch64) {

		std::string code = R"(
			lang aarch64
			section rx

			// random instructions
			label:
			mov x1, 0
			b ne, @label
			mov x0, x30
			mov x8, lr
			ret x8
		)";

		tasml::ErrorHandler reporter {vstl_self.name(), true};
		SegmentedBuffer buffer = tasml::assemble(reporter, code);

		if (!reporter.ok()) {
			reporter.dump();
			FAIL("Errors generated");
		}

	};

	TEST (tasml_check_readme) {

		std::vector<std::string> files {
			"README.md"
		};

		std::regex pattern {"```asm((.|\n)*?)```"};
		std::smatch res;

		for (const auto& file : files) {

			std::ifstream ifs {file};

			if (!ifs.is_open()) {
				FAIL("Failed to read file " + file + ", current working directory is " + std::filesystem::current_path().string());
			}

			std::string content;
			util::load_file_into(ifs, content);

			int suffix = 0;
			auto it = content.cbegin();

			while (std::regex_search(it, content.cend(), res, pattern)) {

				tasml::ErrorHandler reporter {std::string(vstl_self.name()) + "-" + file + "-" + std::to_string(suffix++), true};

				try {
					tasml::assemble(reporter, res.str(1));
				} catch (const std::exception&) {
					reporter.dump();
					throw;
				}

				it = res.suffix().first;
			}

		}

	};

	TEST (tasml_symbol_export) {

		std::string code = R"(
			lang x86
			section rw ".custom"
			tipp: byte 21, 37
			pill: byte 42, 00

			section rx
			export my_strlen:
			export weak strlen:
				mov rcx, /* inline comments! */ rax
				dec rax

				l_strlen_next:
					inc rax
					cmp byte [rax], 0
				jne @l_strlen_next

				sub rax, rcx
				ret

			export @tipp
			export private @pill
		)";

		tasml::ErrorHandler reporter {vstl_self.name(), true};
		SegmentedBuffer buffer = tasml::assemble(reporter, code);

		if (!reporter.ok()) {
			reporter.dump();
			FAIL("Errors generated");
		}

		ElfFile file = to_elf(buffer, "strlen");
		util::TempFile temp {file};

		std::string result = call_shell("readelf -a " + temp.path());

		ASSERT(!result.contains("Warning"));
		ASSERT(!result.contains("Error"));

		ASSERT(result.contains("0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND"))
		ASSERT(result.contains("1: 0000000000000002     0 OBJECT  LOCAL  HIDDEN     2 pill"));
		ASSERT(result.contains("2: 0000000000000000     0 FUNC    GLOBAL PROTECTED    3 my_strlen"));
		ASSERT(result.contains("3: 0000000000000000     0 FUNC    WEAK   PROTECTED    3 strlen"));
		ASSERT(result.contains("4: 0000000000000000     0 OBJECT  GLOBAL PROTECTED    2 tipp"));

	};

	TEST (tasml_embed_statement) {

		std::string embed = "Hello embedded content!\n";
		util::TempFile embedded {".txt"};
		embedded.write(embed);

		std::string code = R"(
			section r
			export private begin:
				embed ")" + embedded.path() + R"("
			export private end:
		)";

		tasml::ErrorHandler reporter {vstl_self.name(), true};
		SegmentedBuffer buffer = tasml::assemble(reporter, code);

		if (!reporter.ok()) {
			reporter.dump();
			FAIL("Errors generated");
		}

		ElfFile file = to_elf(buffer, Label::UNSET);
		util::TempFile temp {file};

		std::string result = call_shell("readelf -a " + temp.path());

		ASSERT(!result.contains("Warning"));
		ASSERT(!result.contains("Error"));

		CHECK(embed.size(), 0x18);

		ASSERT(result.contains("0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND"));
		ASSERT(result.contains("1: 0000000000000000     0 OBJECT  LOCAL  HIDDEN     2 begin"));
		ASSERT(result.contains("2: 0000000000000018     0 OBJECT  LOCAL  HIDDEN     2 end"));

		// very basic test but it's unlikely thet embed specifically broke ELF itself,
		// so if the string is in there we can readable assume everything worked.
		std::string strings = call_shell("strings " + temp.path());
		ASSERT(strings.contains(embed));

	};

	TEST (tasml_embed_missing_file) {

		std::string code = R"(
			section r
			export private begin:
				embed "missing-file.bin"
			export private end:
		)";

		tasml::ErrorHandler reporter {vstl_self.name(), true};

		EXPECT_THROW(std::runtime_error) {
			tasml::assemble(reporter, code);
		};

		CHECK(reporter.ok(), false);

	};

	TEST (tasml_source_mapping) {

		std::string code = R"(
			source "./test/foo.bar" 21 37
			byte 1
		)";

		tasml::ErrorHandler reporter {vstl_self.name(), true};
		auto program = tasml::assemble(reporter, code);
		ASSERT(reporter.ok());

		auto& slocs = program.locations();
		auto& files = program.files();

		CHECK(slocs.size(), 1);
		CHECK(files.size(), 1);

		CHECK(files[0], "./test/foo.bar");
		CHECK(slocs[0].file, 0);
		CHECK(slocs[0].line, 21);
		CHECK(slocs[0].column, 37);

	};

}