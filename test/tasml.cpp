
#define DEBUG_MODE false
#define VSTL_TEST_COUNT 3
#define VSTL_PRINT_SKIP_REASON true
#define VSTL_SUBMODULE true

#include <tasml/top.hpp>

#include "vstl.hpp"
#include "out/buffer/executable.hpp"

namespace test::tas {
	using namespace asmio;

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

		tasml::ErrorHandler reporter {"tasml_tokenize", true};
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

		tasml::ErrorHandler reporter {"tasml_tokenize", true};
		SegmentedBuffer buffer = tasml::assemble(reporter, code);

		if (!reporter.ok()) {
			reporter.dump();
			FAIL("Errors generated");
		}

	};

}