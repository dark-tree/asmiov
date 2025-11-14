#define DEBUG_MODE false
#define VSTL_TEST_COUNT 3
#define VSTL_PRINT_SKIP_REASON true
#define VSTL_SUBMODULE true

#include <util.hpp>
#include <out/buffer/label.hpp>
#include <out/chunk/buffer.hpp>

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

	TEST (util_chunk_buffer_simple) {

		ChunkBuffer buffer;

		// by default we encode as little-endian
		buffer.put<uint32_t>(0xA1A2'A3A4);
		buffer.put<uint8_t>(0xBB);
		buffer.put<uint8_t>(0xCC);

		auto sub = buffer.chunk(8);

		buffer.put<uint8_t>(0xDD);
		buffer.put<uint8_t>(0xEE);

		ChunkBuffer::Ptr outer = std::make_shared<ChunkBuffer>(std::endian::big);
		ChunkBuffer::Ptr inner = outer->chunk();
		outer->put<uint16_t>(0x4455);

		CHECK(inner->root(), outer->root());

		sub->put<uint32_t>(0x1111'1111);

		outer->put<uint8_t>(0x66);

		buffer.adopt(outer);
		CHECK(inner->root(), &buffer); // was the root changed?

		auto bytes = buffer.bake();

		std::vector<uint8_t> expected = {
			0xA4, 0xA3, 0xA2, 0xA1, 0xBB, 0xCC, 0x00, 0x00, 0x11, 0x11, 0x11, 0x11, 0xDD, 0xEE, 0x44, 0x55, 0x66
		};

		CHECK(bytes, expected);

	};

	TEST (util_chunk_buffer_linker) {

		int foo = 0xFF;
		int bar = 0xFF;

		ChunkBuffer buffer {std::endian::big};

		buffer.put<uint8_t>(0x11);
		buffer.put<uint8_t>(0x22);
		buffer.put<uint16_t>(0xAABB);

		buffer.link<uint32_t>([&] { return foo; }); // getter style link
		buffer.link<uint32_t>([&] (auto& target) { target = bar; }); // linker style link (will ignore endianness)

		buffer.put<uint16_t>(0xABAB);

		foo = 0x42;
		bar = 0x53;

		auto bytes = buffer.bake();

		std::vector<uint8_t> expected = {
			0x11, 0x22, 0xAA, 0xBB, 0x00, 0x00, 0x00, 0x42
		};

		if constexpr (std::endian::native == std::endian::little) {
			expected.push_back(bar);
			expected.push_back(0x00);
			expected.push_back(0x00);
			expected.push_back(0x00);
		} else {
			expected.push_back(0x00);
			expected.push_back(0x00);
			expected.push_back(0x00);
			expected.push_back(bar);
		}

		expected.push_back(0xAB);
		expected.push_back(0xAB);

		CHECK(bytes, expected);

	};

	TEST (util_chunk_buffer_offset) {

		ChunkBuffer buffer {std::endian::big};

		buffer.put<uint8_t>(0x11);
		buffer.put<uint8_t>(0x22);

		ChunkBuffer::Ptr a = buffer.chunk();
		ChunkBuffer::Ptr b = buffer.chunk();

		buffer.put<uint8_t>(0xFF);

		CHECK(a->offset(), 2);
		CHECK(b->offset(), 2);

		b->put<uint32_t>(0x01020304);

		CHECK(a->offset(), 2);
		CHECK(b->offset(), 2);

		a->put<uint16_t>(0xAABB);
		a->link<uint16_t>([&] { return b->offset(); });
		a->put<uint16_t>(0x3333);
		a->put<uint16_t>(0x4444);

		CHECK(a->offset(), 2);
		CHECK(b->offset(), 10);

		auto bytes = buffer.bake();

		std::vector<uint8_t> expected = {
			0x11, 0x22, 0xAA, 0xBB, 0x00, 0x0A, 0x33, 0x33, 0x44, 0x44, 0x01, 0x02, 0x03, 0x04, 0xFF
		};

		CHECK(bytes, expected);

	};

	TEST (util_chunk_buffer_relink) {

		int abc = 0x44;

		ChunkBuffer buffer {std::endian::big};

		buffer.put<uint8_t>(0x11);
		buffer.put<uint8_t>(0x22);
		buffer.put<uint8_t>(0x33);

		buffer.link<uint8_t>([&] { return abc; });

		abc = 0x21;
		auto bytes_1 = buffer.bake();
		std::vector<uint8_t> expected_1 = { 0x11, 0x22, 0x33, 0x21 };

		CHECK(bytes_1, expected_1);

		abc = 0x37;
		auto bytes_2 = buffer.bake();
		std::vector<uint8_t> expected_2 = { 0x11, 0x22, 0x33, 0x37 };

		CHECK(bytes_2, expected_2);

	};


}