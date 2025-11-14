#define DEBUG_MODE false
#define VSTL_TEST_COUNT 3
#define VSTL_PRINT_SKIP_REASON true
#define VSTL_SUBMODULE true

#include <util.hpp>
#include <out/buffer/label.hpp>

#include "vstl.hpp"

namespace test::unit {

	using namespace asmio;

	TEST (unit_bit_fill) {

		CHECK(util::bit_fill<uint64_t>(0), 0);
		CHECK(util::bit_fill<uint64_t>(1), 1);
		CHECK(util::bit_fill<uint64_t>(2), 3);
		CHECK(util::bit_fill<uint64_t>(4), 0xf);
		CHECK(util::bit_fill<uint64_t>(8), 0xff);
		CHECK(util::bit_fill<uint64_t>(16), 0xffff);
		CHECK(util::bit_fill<uint64_t>(32), 0xffff'ffff);
		CHECK(util::bit_fill<uint64_t>(48), 0xffff'ffff'ffff);
		CHECK(util::bit_fill<uint64_t>(64), 0xffff'ffff'ffff'ffff);

		CHECK(util::bit_fill<uint32_t>(16), 0xffff);
		CHECK(util::bit_fill<uint32_t>(32), 0xffff'ffff);
		CHECK(util::bit_fill<uint32_t>(48), 0xffff'ffff);
		CHECK(util::bit_fill<uint32_t>(64), 0xffff'ffff);

	};

	TEST (unit_label_string_view) {

		std::string base = "aaabbb";
		std::string_view view = base;

		std::string_view view_a = view.substr(0, 3);
		std::string_view view_b = view.substr(3, 3);

		Label label_a {"aaa"};
		Label label_b {"bbb"};

		CHECK(label_a, Label(view_a));
		CHECK(label_b, Label(view_b));

	};

	TEST (unit_min_unsigned_integer_bytes) {

		CHECK(util::min_bytes(0xFF), 1);
		CHECK(util::min_bytes(0x123456), 4);
		CHECK(util::min_bytes(0xF000), 2);
		CHECK(util::min_bytes(0x1888888888), 8);
		CHECK(util::min_bytes(0), 1);

	};

	TEST (unit_min_extended_integer_bytes) {

		CHECK(util::min_sign_extended_bytes(0), 1);
		CHECK(util::min_sign_extended_bytes(-0x11), 1);
		CHECK(util::min_sign_extended_bytes(0x123456), 4);
		CHECK(util::min_sign_extended_bytes(0x1888888888), 8);
		CHECK(util::min_sign_extended_bytes(0xFFFF'FF01), 8);
		CHECK(util::min_sign_extended_bytes(0x7FFF'FF01), 4);
		CHECK(util::min_sign_extended_bytes(0x80), 2);
		CHECK(util::min_sign_extended_bytes(0xFFFF), 4);

	};

}