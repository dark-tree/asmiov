
#define DEBUG_MODE false
#define VSTL_TEST_COUNT 3
#define VSTL_PRINT_SKIP_REASON true
#define VSTL_SUBMODULE true

#include <tasml/top.hpp>

#include "vstl.hpp"
#include "out/buffer/executable.hpp"

namespace test::tas {
	using namespace asmio;

	TEST (tasml_tokenize) {

		std::string code = R"(
			lang x86

			text:
				byte "Hello!", 0

			strlen:
				mov rcx, /* inline comments! */ rax
				dec rax

				l_strlen_next:
					inc rax
					cmp byte [rax], 0
				jne @l_strlen_next

				sub rax, rcx
				ret

			_start:
				lea rax, @text
				call @strlen
				nop; nop; ret // multi-statements
		)";

		tasml::ErrorHandler reporter {"tasml_tokenize", true};

		try {
			SegmentedBuffer buffer = tasml::assemble(reporter, code);
			CHECK(to_executable(buffer).call_i32("_start"), 6);
		} catch (std::runtime_error& e) {}

		if (!reporter.ok()) {
			reporter.dump();
			FAIL("Errors generated");
		}

	};

}