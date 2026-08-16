
#include "linkage.hpp"
#include <asmio/util/bits.hpp>
#include <asmio/util/string.hpp>

namespace asmio {

	static void encode_21_5_lo_hi(SegmentedBuffer* buffer, const Linkage& linkage, BufferMarker src, size_t mount) {
		BufferMarker dst = linkage.target;

		const int64_t offset = buffer->get_offset(src) - buffer->get_offset(dst);

		if (!util::is_signed_encodable(offset, 21)) {
			throw std::runtime_error {"Can't fit label '" + linkage.label.string() + "' (offset " + util::to_hex(offset) + ") into target " + util::to_hex(dst.offset) + ", some data would have been truncated!"};
		}

		const uint64_t masked = util::bit_fill<uint64_t>(21) & offset;
		const uint32_t immlo = masked & 0b11;
		const uint32_t immhi = masked >> 2;

		*reinterpret_cast<uint32_t*>(buffer->get_pointer(dst)) |= (immlo << 29 | immhi << 5);
	}

	template <int bits, int left_shift>
	static void encode_shifted_aligned(SegmentedBuffer* buffer, const Linkage& linkage, BufferMarker src, size_t mount) {
		BufferMarker dst = linkage.target;

		const int64_t distance = buffer->get_offset(src) - buffer->get_offset(dst) + linkage.addend;

		if (distance & 0b11) {
			throw std::runtime_error {"Can't reference label '" + linkage.label.string() + "' (offset " + util::to_hex(distance) + ") into target " + util::to_hex(dst.offset) + ", offset is not 4-aligned!"};
		}

		const int64_t offset = distance >> 2;

		if (!util::is_signed_encodable(offset, bits)) {
			throw std::runtime_error {"Can't fit label '" + linkage.label.string() + "' (offset " + util::to_hex(distance) + ") into target " + util::to_hex(dst.offset) + ", some data would have been truncated!"};
		}

		*reinterpret_cast<uint32_t*>(buffer->get_pointer(dst)) |= ((util::bit_fill<uint64_t>(bits) & offset) << left_shift);
	}

	template <int width>
	static void encode_relative(SegmentedBuffer* buffer, const Linkage& linkage, BufferMarker src, size_t mount) {
		BufferMarker dst = linkage.target;

		const int64_t value = buffer->get_offset(src) - buffer->get_offset(dst) + linkage.addend;

		if (util::min_signed_bytes(value) > width) {
			throw std::runtime_error {"Can't fit label '" + linkage.label.string() + "' (" + util::to_hex(value) + ") into target of size " + std::to_string(width) + ", some data would have been truncated!"};
		}

		const uint8_t* value_ptr = reinterpret_cast<const uint8_t*>(&value);
		std::memcpy(buffer->get_pointer(dst), value_ptr, width);
	}

	template <int width>
	static void encode_absolute(SegmentedBuffer* buffer, const Linkage& linkage, BufferMarker src, size_t mount) {
		BufferMarker dst = linkage.target;

		const int64_t value = buffer->get_offset(src) + mount + linkage.addend;

		if (util::min_unsigned_bytes(value) > width) {
			throw std::runtime_error {"Can't fit label '" + linkage.label.string() + "' (" + util::to_hex(value) + ") into target of size " + std::to_string(width) + ", some data would have been truncated!"};
		}

		const uint8_t* value_ptr = reinterpret_cast<const uint8_t*>(&value);
		std::memcpy(buffer->get_pointer(dst), value_ptr, width);
	}

	static void encode_riscv_b(SegmentedBuffer* buffer, const Linkage& linkage, BufferMarker src, size_t mount) {
		BufferMarker dst = linkage.target;
		const int64_t dist = buffer->get_offset(src) - buffer->get_offset(dst) + linkage.addend;

		if (dist & 1) {
			throw std::runtime_error {"Can't reference label '" + linkage.label.string() + "' (offset " + util::to_hex(dist) + ") into target " + util::to_hex(dst.offset) + ", offset is not 2-aligned!"};
		}

		const int64_t imm12 = dist >> 1;

		if (!util::is_signed_encodable(imm12, 12)) {
			throw std::runtime_error {"Can't fit label '" + linkage.label.string() + "' (offset " + util::to_hex(dist) + ") into target " + util::to_hex(dst.offset) + ", some data would have been truncated!"};
		}

		const uint32_t si = (imm12 & 0b1'0'000000'0000) >> 11;
		const uint32_t b7 = (imm12 & 0b0'1'000000'0000) >> 10;
		const uint32_t hi = (imm12 & 0b0'0'111111'0000) >> 4;
		const uint32_t lo = (imm12 & 0b0'0'000000'1111) >> 0;

		*reinterpret_cast<uint32_t*>(buffer->get_pointer(dst)) |= (si << 31 | hi << 25 | lo << 8 | b7 << 7);
	}

	static void encode_riscv_j(SegmentedBuffer* buffer, const Linkage& linkage, BufferMarker src, size_t mount) {
		BufferMarker dst = linkage.target;
		const int64_t dist = buffer->get_offset(src) - buffer->get_offset(dst) + linkage.addend;

		if (dist & 1) {
			throw std::runtime_error {"Can't reference label '" + linkage.label.string() + "' (offset " + util::to_hex(dist) + ") into target " + util::to_hex(dst.offset) + ", offset is not 2-aligned!"};
		}

		const int64_t imm20 = dist >> 1;

		if (!util::is_signed_encodable(imm20, 20)) {
			throw std::runtime_error {"Can't fit label '" + linkage.label.string() + "' (offset " + util::to_hex(dist) + ") into target " + util::to_hex(dst.offset) + ", some data would have been truncated!"};
		}

		const uint32_t si = (imm20 & 0b1'00000000'0'0000000000) >> 19;
		const uint32_t hi = (imm20 & 0b0'11111111'0'0000000000) >> 11;
		const uint32_t b2 = (imm20 & 0b0'00000000'1'0000000000) >> 10;
		const uint32_t lo = (imm20 & 0b0'00000000'0'1111111111) >> 0;

		*reinterpret_cast<uint32_t*>(buffer->get_pointer(dst)) |= (si << 31 | lo << 21 | b2 << 20 | hi << 12);
	}

	/*
	 * class LinkageType
	 */

	Linkage::Type LinkageType::RISCV_BRANCH {ElfRelocationType::RISCV_BRANCH, encode_riscv_b};
	Linkage::Type LinkageType::RISCV_JUMP {ElfRelocationType::RISCV_JAL, encode_riscv_j};

	Linkage::Type LinkageType::AARCH64_21_5_LO_HI {ElfRelocationType::AARCH64_ADR_PREL_LO21, encode_21_5_lo_hi};
	Linkage::Type LinkageType::AARCH64_14_5_ALIGNED {ElfRelocationType::AARCH64_TSTBR14, encode_shifted_aligned<14, 5>};
	Linkage::Type LinkageType::AARCH64_19_5_ALIGNED {ElfRelocationType::AARCH64_CONDBR19, encode_shifted_aligned<19, 5>};
	Linkage::Type LinkageType::AARCH64_26_0_ALIGNED {ElfRelocationType::AARCH64_JUMP26, encode_shifted_aligned<26, 0>};

	Linkage::Type LinkageType::X86_64_ABSOLUTE {ElfRelocationType::X86_64_64, encode_absolute<8>};
	Linkage::Type LinkageType::X86_64_RELATIVE {ElfRelocationType::X86_64_PC64, encode_relative<8>};
	Linkage::Type LinkageType::X86_32_ABSOLUTE {ElfRelocationType::X86_64_32, encode_absolute<4>};
	Linkage::Type LinkageType::X86_32_SIGN_ABSOLUTE {ElfRelocationType::X86_64_32S, encode_absolute<4>};
	Linkage::Type LinkageType::X86_32_SIGN_RELATIVE {ElfRelocationType::X86_64_PC32, encode_relative<4>};
	Linkage::Type LinkageType::X86_16_ABSOLUTE {ElfRelocationType::X86_64_16, encode_absolute<2>};
	Linkage::Type LinkageType::X86_16_SIGN_RELATIVE {ElfRelocationType::X86_64_PC16, encode_relative<2>};
	Linkage::Type LinkageType::X86_8_SIGN_ABSOLUTE {ElfRelocationType::X86_64_8, encode_absolute<1>};
	Linkage::Type LinkageType::X86_8_SIGN_RELATIVE {ElfRelocationType::X86_64_PC8, encode_relative<1>};

}