
#include "linkage.hpp"

namespace asmio {

	static void encode_21_5_lo_hi(SegmentedBuffer* buffer, const Linkage& linkage, size_t mount) {
		BufferMarker src = buffer->get_label(linkage.label);
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

	static void encode_shifted_aligned_link(SegmentedBuffer* buffer, const Linkage& linkage, int bits, int left_shift) {
		BufferMarker src = buffer->get_label(linkage.label);
		BufferMarker dst = linkage.target;

		const int64_t distance = buffer->get_offset(src) - buffer->get_offset(dst) + linkage.addend;

		if (distance & 0b11) {
			throw std::runtime_error {"Can't reference label '" + linkage.label.string() + "' (offset " + util::to_hex(distance) + ") into target " + util::to_hex(dst.offset) + ", offset is not aligned!"};
		}

		const int64_t offset = distance >> 2;

		if (!util::is_signed_encodable(offset, bits)) {
			throw std::runtime_error {"Can't fit label '" + linkage.label.string() + "' (offset " + util::to_hex(distance) + ") into target " + util::to_hex(dst.offset) + ", some data would have been truncated!"};
		}

		*reinterpret_cast<uint32_t*>(buffer->get_pointer(dst)) |= ((util::bit_fill<uint64_t>(bits) & offset) << left_shift);
	}

	static void encode_26_0_aligned(SegmentedBuffer* buffer, const Linkage& linkage, size_t mount) {
		encode_shifted_aligned_link(buffer, linkage, 26, 0);
	}

	static void encode_19_5_aligned(SegmentedBuffer* buffer, const Linkage& linkage, size_t mount) {
		encode_shifted_aligned_link(buffer, linkage, 19, 5);
	}

	static void encode_14_5_aligned(SegmentedBuffer* buffer, const Linkage& linkage, size_t mount) {
		encode_shifted_aligned_link(buffer, linkage, 14, 5);
	}

	template <size_t width>
	static void encode_relative(SegmentedBuffer* buffer, const Linkage& linkage, size_t mount) {
		BufferMarker src = buffer->get_label(linkage.label);
		BufferMarker dst = linkage.target;

		const int64_t value = buffer->get_offset(src) - buffer->get_offset(dst) + linkage.addend;

		if (util::min_sign_extended_bytes(value) > width) {
			throw std::runtime_error {"Can't fit label '" + linkage.label.string() + "' (" + util::to_hex(value) + ") into target of size " + std::to_string(width) + ", some data would have been truncated!"};
		}

		const uint8_t* value_ptr = reinterpret_cast<const uint8_t*>(&value);
		memcpy(buffer->get_pointer(dst), value_ptr, width);
	}

	template <int width>
	static void encode_absolute(SegmentedBuffer* buffer, const Linkage& linkage, size_t mount) {
		BufferMarker src = buffer->get_label(linkage.label);
		BufferMarker dst = linkage.target;

		const int64_t value = buffer->get_offset(src) + mount + linkage.addend;

		if (util::min_sign_extended_bytes(value) > width) {
			throw std::runtime_error {"Can't fit label '" + linkage.label.string() + "' (" + util::to_hex(value) + ") into target of size " + std::to_string(width) + ", some data would have been truncated!"};
		}

		const uint8_t* value_ptr = reinterpret_cast<const uint8_t*>(&value);
		memcpy(buffer->get_pointer(dst), value_ptr, width);
	}

	/*
	 * class LinkageType
	 */

	Linkage::Type LinkageType::AARCH64_21_5_LO_HI {ElfRelocationType::AARCH64_ADR_PREL_LO21, encode_21_5_lo_hi};
	Linkage::Type LinkageType::AARCH64_14_5_ALIGNED {ElfRelocationType::AARCH64_TSTBR14, encode_14_5_aligned};
	Linkage::Type LinkageType::AARCH64_19_5_ALIGNED {ElfRelocationType::AARCH64_CONDBR19, encode_19_5_aligned};
	Linkage::Type LinkageType::AARCH64_26_0_ALIGNED {ElfRelocationType::AARCH64_JUMP26, encode_26_0_aligned};

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