#define DEBUG_MODE false
#define VSTL_TEST_COUNT 3
#define VSTL_PRINT_SKIP_REASON true
#define VSTL_SUBMODULE true

#include <util.hpp>
#include <out/buffer/label.hpp>
#include <out/buffer/segmented.hpp>
#include <out/chunk/buffer.hpp>
#include <out/chunk/codecs.hpp>

#include "vstl.hpp"

namespace test {

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

	TEST (util_is_sign_encodable) {

		CHECK(util::is_signed_encodable(-100, 8), true);
		CHECK(util::is_signed_encodable(-128, 8), true);
		CHECK(util::is_signed_encodable(-129, 8), false);
		CHECK(util::is_signed_encodable(127, 8), true);
		CHECK(util::is_signed_encodable(128, 8), false);

	};

	TEST (util_chunk_buffer_simple) {

		ChunkBuffer buffer;

		// by default, we encode as little-endian
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

		buffer.link<uint32_t>([&] noexcept { return foo; }); // getter style link
		buffer.link<uint32_t>([&] (auto& target) noexcept { target = bar; }); // linker style link (will ignore endianness)

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

		buffer.link<uint8_t>([&] noexcept { return abc; });

		abc = 0x21;
		auto bytes_1 = buffer.bake();
		std::vector<uint8_t> expected_1 = { 0x11, 0x22, 0x33, 0x21 };

		CHECK(bytes_1, expected_1);

		abc = 0x37;
		auto bytes_2 = buffer.bake();
		std::vector<uint8_t> expected_2 = { 0x11, 0x22, 0x33, 0x37 };

		CHECK(bytes_2, expected_2);

	};

	TEST (util_parse_int) {

		CHECK(util::parse_int("0"), 0);
		CHECK(util::parse_int("+1000"), 1000);
		CHECK(util::parse_int("-1000"), -1000);
		CHECK(util::parse_int("0xFEB00000"), 0xFEB00000);
		CHECK(util::parse_int("0xFAFFFFFFFBFFFFFE"), 0xFAFFFFFFFBFFFFFE);
		CHECK(util::parse_int("0b1010101"), 0b1010101);
		CHECK(util::parse_int("-0b1010101"), -0b1010101);

	}

	TEST (util_source_location) {

		SegmentedBuffer program;

		program.add_location("./my/foo.txt", 1, 1);
		program.push(1);
		program.push(2);
		program.push(3);
		program.push(4);

		program.add_location("./my/bar.txt", 1, 1);
		program.push(5);
		program.push(6);

		program.add_location("./my/foo.txt", 2, 10);
		program.push(7);
		program.push(8);

		const auto& locations = program.locations();
		const auto& files = program.files();

		CHECK(files.size(), 2);
		CHECK(locations.size(), 3);

		CHECK(files[0], "./my/foo.txt");
		CHECK(files[1], "./my/bar.txt");

		CHECK(locations[0].file, 0);
		CHECK(locations[1].file, 1);
		CHECK(locations[2].file, 0);

	};

	TEST (util_codec_uleb128) {

		{
			ChunkBuffer buffer {};
			buffer.put<UnsignedLeb128>(13);
			auto bytes = buffer.bake();
			CHECK(bytes.size(), 1);
			CHECK(bytes[0], 13);
		}

		{
			ChunkBuffer buffer {};
			buffer.put<UnsignedLeb128>(127);
			auto bytes = buffer.bake();
			CHECK(bytes.size(), 1);
			CHECK(bytes[0], 127);
		}

		{
			ChunkBuffer buffer {};
			buffer.put<UnsignedLeb128>(128);
			auto bytes = buffer.bake();
			CHECK(bytes.size(), 2);
			CHECK(bytes[0], 128);
			CHECK(bytes[1], 1);
		}

		{ // from wikipedia
			ChunkBuffer buffer {};
			buffer.put<UnsignedLeb128>(624485);
			auto bytes = buffer.bake();
			CHECK(bytes.size(), 3);
			CHECK(bytes[0], 0xE5);
			CHECK(bytes[1], 0x8E);
			CHECK(bytes[2], 0x26);
		}

	};

	TEST (util_codec_sleb128) {

		{
			ChunkBuffer buffer {};
			buffer.put<SignedLeb128>(13);
			auto bytes = buffer.bake();
			CHECK(bytes.size(), 1);
			CHECK(bytes[0], 13);
		}

		{
			ChunkBuffer buffer {};
			buffer.put<SignedLeb128>(-1);
			auto bytes = buffer.bake();
			CHECK(bytes.size(), 1);
			CHECK(bytes[0], 0x7f);
		}

		{
			ChunkBuffer buffer {};
			buffer.put<SignedLeb128>(128);
			auto bytes = buffer.bake();
			CHECK(bytes.size(), 2);
			CHECK(bytes[0], 128);
			CHECK(bytes[1], 1);
		}

		{ // from wikipedia
			ChunkBuffer buffer {};
			buffer.put<SignedLeb128>(-123456);
			auto bytes = buffer.bake();
			CHECK(bytes.size(), 3);
			CHECK(bytes[0], 0xC0);
			CHECK(bytes[1], 0xBB);
			CHECK(bytes[2], 0x78);
		}

	};

	TEST (util_refcnt) {

		int* buffer = ref_allocate<int>(4);

		buffer[0] = 21;
		buffer[1] = 37;
		buffer[2] = 19;
		buffer[3] = 23;

		CHECK(ref_count(buffer), 1);
		CHECK(buffer[0], 21);
		CHECK(buffer[3], 23);

		ref_increment(buffer);

		CHECK(ref_count(buffer), 2);
		CHECK(buffer[0], 21);
		CHECK(buffer[3], 23);

		ASSERT(!ref_free(buffer));

		CHECK(ref_count(buffer), 1);
		CHECK(buffer[0], 21);
		CHECK(buffer[3], 23);

		ASSERT(ref_free(buffer));

	};

}