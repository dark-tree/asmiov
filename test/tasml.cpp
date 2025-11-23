
#define DEBUG_MODE false
#define VSTL_TEST_COUNT 3
#define VSTL_PRINT_SKIP_REASON true
#define VSTL_SUBMODULE true

#include <tasml/top.hpp>

#include "vstl.hpp"
#include "out/buffer/executable.hpp"

namespace test::tas {
	using namespace asmio;

	TEST (util_parse_int) {

		CHECK(util::parse_int("0"), 0);
		CHECK(util::parse_int("+1000"), 1000);
		CHECK(util::parse_int("-1000"), -1000);
		CHECK(util::parse_int("0xFEB00000"), 0xFEB00000);
		CHECK(util::parse_int("0xFAFFFFFFFBFFFFFE"), 0xFAFFFFFFFBFFFFFE);
		CHECK(util::parse_int("0b1010101"), 0b1010101);
		CHECK(util::parse_int("-0b1010101"), -0b1010101);

	}

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

}