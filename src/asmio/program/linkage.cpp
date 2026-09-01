
#include "linkage.hpp"
#include <asmio/util/bits.hpp>
#include <asmio/util/string.hpp>

namespace asmio {

	[[noreturn]]
	static void it_wont_fit(const Linkage& linkage, int64_t value, int64_t dst, int64_t size) {
		throw std::runtime_error {"Can't fit label '" + linkage.label.string() + "' (resolved to " + util::to_hex(value) + ") into target " + util::to_hex(dst) + " of size " + std::to_string(size) + ", some data would have been truncated!"};
	}

	[[noreturn]]
	static void it_isnt_aligned(const Linkage& linkage, int64_t value, int64_t dst, int64_t alignment) {
		throw std::runtime_error {"Can't reference label '" + linkage.label.string() + "' (resolved to " + util::to_hex(value) + ") into target " + util::to_hex(dst) + ", value is not " + std::to_string(alignment) + "-aligned!"};
	}

	/*
	 * Linker Implementation
	 */

	static void encode_21_5_lo_hi(SegmentedBuffer* buffer, const Linkage& linkage, BufferMarker src, size_t) {
		BufferMarker dst = linkage.target;

		const int64_t offset = buffer->get_offset(src) - buffer->get_offset(dst);

		if (!util::is_signed_encodable(offset, 21)) {
			it_wont_fit(linkage, offset, dst.offset, 21);
		}

		const uint64_t masked = util::bit_fill<uint64_t>(21) & offset;
		const uint32_t immlo = masked & 0b11;
		const uint32_t immhi = masked >> 2;

		*reinterpret_cast<uint32_t*>(buffer->get_pointer(dst)) |= (immlo << 29 | immhi << 5);
	}

	template <int bits, int left_shift>
	static void encode_shifted_aligned(SegmentedBuffer* buffer, const Linkage& linkage, BufferMarker src, size_t) {
		BufferMarker dst = linkage.target;

		const int64_t distance = buffer->get_offset(src) - buffer->get_offset(dst) + linkage.addend;

		if (distance & 0b11) {
			it_isnt_aligned(linkage, distance, dst.offset, 4);
		}

		const int64_t offset = distance >> 2;

		if (!util::is_signed_encodable(offset, bits)) {
			it_wont_fit(linkage, offset, dst.offset, bits);
		}

		*reinterpret_cast<uint32_t*>(buffer->get_pointer(dst)) |= ((util::bit_fill<uint64_t>(bits) & offset) << left_shift);
	}

	template <int width>
	static void encode_relative(SegmentedBuffer* buffer, const Linkage& linkage, BufferMarker src, size_t) {
		BufferMarker dst = linkage.target;

		const int64_t value = buffer->get_offset(src) - buffer->get_offset(dst) + linkage.addend;

		if (util::min_signed_bytes(value) > width) {
			it_wont_fit(linkage, value, dst.offset, width * 8);
		}

		const uint8_t* value_ptr = reinterpret_cast<const uint8_t*>(&value);
		std::memcpy(buffer->get_pointer(dst), value_ptr, width);
	}

	template <int width>
	static void encode_absolute(SegmentedBuffer* buffer, const Linkage& linkage, BufferMarker src, size_t mount) {
		BufferMarker dst = linkage.target;

		const int64_t value = buffer->get_offset(src) + mount + linkage.addend;

		if (util::min_unsigned_bytes(value) > width) {
			it_wont_fit(linkage, value, dst.offset, width * 8);
		}

		const uint8_t* value_ptr = reinterpret_cast<const uint8_t*>(&value);
		std::memcpy(buffer->get_pointer(dst), value_ptr, width);
	}

	static void encode_riscv_b(SegmentedBuffer* buffer, const Linkage& linkage, BufferMarker src, size_t) {
		BufferMarker dst = linkage.target;
		const int64_t dist = buffer->get_offset(src) - buffer->get_offset(dst) + linkage.addend;

		if (dist & 1) {
			it_isnt_aligned(linkage, dist, dst.offset, 2);
		}

		const int64_t imm12 = dist >> 1;

		if (!util::is_signed_encodable(imm12, 12)) {
			it_wont_fit(linkage, imm12, dst.offset, 12);
		}

		const uint32_t si = (imm12 & 0b1'0'000000'0000) >> 11;
		const uint32_t b7 = (imm12 & 0b0'1'000000'0000) >> 10;
		const uint32_t hi = (imm12 & 0b0'0'111111'0000) >> 4;
		const uint32_t lo = (imm12 & 0b0'0'000000'1111) >> 0;

		*reinterpret_cast<uint32_t*>(buffer->get_pointer(dst)) |= (si << 31 | hi << 25 | lo << 8 | b7 << 7);
	}

	static void encode_riscv_j(SegmentedBuffer* buffer, const Linkage& linkage, BufferMarker src, size_t ) {
		BufferMarker dst = linkage.target;
		const int64_t dist = buffer->get_offset(src) - buffer->get_offset(dst) + linkage.addend;

		if (dist & 1) {
			it_isnt_aligned(linkage, dist, dst.offset, 2);
		}

		const int64_t imm20 = dist >> 1;

		if (!util::is_signed_encodable(imm20, 20)) {
			it_wont_fit(linkage, imm20, dst.offset, 20);
		}

		const uint32_t si = (imm20 & 0b1'00000000'0'0000000000) >> 19;
		const uint32_t hi = (imm20 & 0b0'11111111'0'0000000000) >> 11;
		const uint32_t b2 = (imm20 & 0b0'00000000'1'0000000000) >> 10;
		const uint32_t lo = (imm20 & 0b0'00000000'0'1111111111) >> 0;

		*reinterpret_cast<uint32_t*>(buffer->get_pointer(dst)) |= (si << 31 | lo << 21 | b2 << 20 | hi << 12);
	}

	template <uint32_t part, uint32_t bits, uint32_t left_shift, uint32_t sign_mask>
	static void encode_riscv_relative_shifted(SegmentedBuffer* buffer, const Linkage& linkage, BufferMarker src, size_t) {
		BufferMarker dst = linkage.target;
		const int64_t dist = buffer->get_offset(src) - buffer->get_offset(dst) + linkage.addend + 4;

		if (!util::is_signed_encodable(dist, 32)) {
			it_wont_fit(linkage, dist, dst.offset, 32);
		}

		const int64_t value = ((dist >> part) & util::bit_fill<int64_t>(bits)) + (dist & sign_mask ? 1 : 0);
		*reinterpret_cast<uint32_t*>(buffer->get_pointer(dst)) |= (value << left_shift);
	}

	/*
	 * class LinkageType
	 */

	Linkage::Type LinkageType::RISCV_BRANCH {ElfRelocationType::RISCV_BRANCH, Linkage::RELATIVE, encode_riscv_b};
	Linkage::Type LinkageType::RISCV_JUMP {ElfRelocationType::RISCV_JAL, Linkage::RELATIVE, encode_riscv_j};
	Linkage::Type LinkageType::RISCV_PCREL_HI20 {ElfRelocationType::RISCV_PCREL_HI20, Linkage::RELATIVE, encode_riscv_relative_shifted<12, 20, 12, 0x0000'0800>};
	Linkage::Type LinkageType::RISCV_PCREL_LO12 {ElfRelocationType::RISCV_PCREL_LO12_I, Linkage::RELATIVE, encode_riscv_relative_shifted<0, 12, 20, 0x0000'0000>};

	Linkage::Type LinkageType::AARCH64_21_5_LO_HI {ElfRelocationType::AARCH64_ADR_PREL_LO21, Linkage::RELATIVE, encode_21_5_lo_hi};
	Linkage::Type LinkageType::AARCH64_14_5_ALIGNED {ElfRelocationType::AARCH64_TSTBR14, Linkage::RELATIVE, encode_shifted_aligned<14, 5>};
	Linkage::Type LinkageType::AARCH64_19_5_ALIGNED {ElfRelocationType::AARCH64_CONDBR19, Linkage::RELATIVE, encode_shifted_aligned<19, 5>};
	Linkage::Type LinkageType::AARCH64_26_0_ALIGNED {ElfRelocationType::AARCH64_JUMP26, Linkage::RELATIVE, encode_shifted_aligned<26, 0>};

	Linkage::Type LinkageType::X86_64_ABSOLUTE {ElfRelocationType::X86_64_64, Linkage::ABSOLUTE, encode_absolute<8>};
	Linkage::Type LinkageType::X86_64_RELATIVE {ElfRelocationType::X86_64_PC64, Linkage::RELATIVE, encode_relative<8>};
	Linkage::Type LinkageType::X86_32_ABSOLUTE {ElfRelocationType::X86_64_32, Linkage::ABSOLUTE, encode_absolute<4>};
	Linkage::Type LinkageType::X86_32_SIGN_ABSOLUTE {ElfRelocationType::X86_64_32S, Linkage::ABSOLUTE, encode_absolute<4>};
	Linkage::Type LinkageType::X86_32_SIGN_RELATIVE {ElfRelocationType::X86_64_PC32, Linkage::RELATIVE, encode_relative<4>};
	Linkage::Type LinkageType::X86_16_ABSOLUTE {ElfRelocationType::X86_64_16, Linkage::ABSOLUTE, encode_absolute<2>};
	Linkage::Type LinkageType::X86_16_SIGN_RELATIVE {ElfRelocationType::X86_64_PC16, Linkage::RELATIVE, encode_relative<2>};
	Linkage::Type LinkageType::X86_8_SIGN_ABSOLUTE {ElfRelocationType::X86_64_8, Linkage::ABSOLUTE, encode_absolute<1>};
	Linkage::Type LinkageType::X86_8_SIGN_RELATIVE {ElfRelocationType::X86_64_PC8, Linkage::RELATIVE, encode_relative<1>};

}