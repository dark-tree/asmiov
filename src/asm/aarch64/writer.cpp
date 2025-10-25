#include "writer.hpp"

namespace asmio::arm {

	/*
	 * class BufferWriter
	 */

	BufferWriter::BufferWriter(SegmentedBuffer& buffer)
		: BasicBufferWriter(buffer) {
	}

	std::optional<uint16_t> BufferWriter::compute_immediate_bitmask(uint64_t value, bool wide) {

		if (!wide) {

			// this is a interesting case - a 64 bit immediate pattern used for a 32 bit operation, we could just allow it...
			if (value > UINT32_MAX) return std::nullopt;

			// copy the lower bits up to maintain tha pattern across the whole 64 bit number
			value |= value << 32;
		}

		if (value == 0 || value == std::numeric_limits<uint64_t>::max()) {
			return std::nullopt;
		}

		for (uint64_t size : {2, 4, 8, 16, 32, 64}) {

			if (size == 64 && !wide) break;

			// mask all bits smaller or equal the one selected
			const auto mask = util::bit_fill<uint64_t>(size);

			// the first element, we compare all others against this one
			// for 64 bit values this will just be used as-is
			const uint64_t pattern = value & mask;

			// the whole value would need to be zero,
			// this is just a simple short path for simple cases
			if (pattern == 0) {
				continue;
			}

			uint64_t source = value;
			bool matched = true;

			for (size_t i = size; i < 64; i += size) {
				source >>= size;

				// if any element doesn't match this is not the valid element size
				// or the value is not encodable - but we can't tell that right now
				if ((source & mask) != pattern) {
					matched = false;
					break;
				}
			}

			if (matched) {

				// we now know the size, but can't yet tell if the actual pattern is encodable
				// nor the correct rotation that needs to be applied, delegate that to the next function
				return compute_element_bitmask(value, size);
			}
		}

		// realistically this can only happen for non-wide numbers
		return std::nullopt;
	}

	std::optional<uint16_t> BufferWriter::compute_element_bitmask(uint64_t value, size_t size) {

		const auto mask = util::bit_fill<uint64_t>(size);
		const uint64_t pattern = value & mask;
		const int bits = std::popcount(pattern);

		// shift register
		uint64_t shift = value;

		for (size_t i = 0; i < size; i ++) {

			// if all bits are trailing that means the current
			// rotation positioned all of them at the end, creating a valid pattern
			if (util::count_trailing_ones(shift) == bits) {
				return pack_bitmask(size, bits, i);
			}

			shift = std::rotl(shift, 1);
		}

		return std::nullopt;
	}

	uint16_t BufferWriter::pack_bitmask(uint32_t size, uint32_t ones, uint32_t roll) {

		// N | imms        | size    | run-of-ones
		// - + ----------- + ------- + -----------
		// 0 | 1 1 1 1 0 x | 2 bits  | 1
		// 0 | 1 1 1 0 x x | 4 bits  | 1-3
		// 0 | 1 1 0 x x x | 8 bits  | 1-7
		// 0 | 1 0 x x x x | 16 bits | 1-15
		// 0 | 0 x x x x x | 32 bits | 1-31
		// 1 | x x x x x x | 64 bits | 1-63
		uint32_t nimms = 0b0'111111;

		// clears the '0' bit separating 'size' from 'run-of-ones'
		// this will ALSO set the N to 1 for 64 bit numbers, as that is the only non-1 bit
		nimms ^= size;

		// next we clear all the 'x' bits (set them to 0) by
		// ANDing them with the inverse of the mask
		nimms &= ~(size - 1);

		// at this point we can encode the run-of-ones value by ORing it with the nimms
		// do note that the encoded value is decremented by one, so run length 1 is saved as 0, all ones length is not valid
		nimms |= (ones - 1);

		// as the N bit is saved separately we have to extract it back from our value
		const uint32_t n = (nimms & 0b1'000000) >> 6;

		// same here, we mask out the N from nimms to get the imms part
		return n << 12 | roll << 6 | (nimms & 0b0'111111);
	}

	void BufferWriter::put_inst_bitmask(uint32_t opc_from_23, Registry destination, Registry source, uint16_t n_immr_imms) {
		uint16_t sf = destination.wide() ? 1 : 0;
		put_dword(sf << 31 | opc_from_23 << 23 | n_immr_imms << 10 | source.reg << 5 | destination.reg);
	}

	void BufferWriter::encode_shifted_aligned_link(SegmentedBuffer* buffer, const Linkage& linkage, int bits, int left_shift) {
		BufferMarker src = buffer->get_label(linkage.label);
		BufferMarker dst = linkage.target;

		const int64_t distance = buffer->get_offset(src) - buffer->get_offset(dst);

		if (distance & 0b11) {
			throw std::runtime_error {"Can't reference label '" + linkage.label.string() + "' (offset " + util::to_hex(distance) + ") into target " + util::to_hex(dst.offset) + ", offset is not aligned!"};
		}

		const int64_t offset = distance >> 2;

		if (!util::is_signed_encodable(offset, bits)) {
			throw std::runtime_error {"Can't fit label '" + linkage.label.string() + "' (offset " + util::to_hex(distance) + ") into target " + util::to_hex(dst.offset) + ", some data would have been truncated!"};
		}

		*reinterpret_cast<uint32_t*>(buffer->get_pointer(dst)) |= ((util::bit_fill<uint64_t>(bits) & offset) << left_shift);
	}

	void BufferWriter::link_26_0_aligned(SegmentedBuffer* buffer, const Linkage& linkage, size_t mount) {
		encode_shifted_aligned_link(buffer, linkage, 26, 0);
	}

	void BufferWriter::link_19_5_aligned(SegmentedBuffer* buffer, const Linkage& linkage, size_t mount) {
		encode_shifted_aligned_link(buffer, linkage, 19, 5);
	}

	void BufferWriter::link_21_5_lo_hi(SegmentedBuffer* buffer, const Linkage& linkage, size_t mount) {
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

	uint8_t BufferWriter::pack_shift(uint8_t shift, bool wide) {
		if (shift & 0b0000'1111) throw std::runtime_error {"Invalid shift, only multiples of 16 allowed"};
		if (shift & 0b1100'0000) throw std::runtime_error {"Invalid shift, the maximum value of 48 exceeded"};

		const uint8_t hw = shift >> 4;
		if (!wide && hw > 1) throw std::runtime_error {"Invalid shift, only 0 or 16 bit shifts allowed in 32 bit context"};

		return hw;
	}

	void BufferWriter::assert_register_triplet(Registry a, Registry b, Registry c) {
		if (a.wide() != b.wide() || a.wide() != c.wide()) {
			throw std::runtime_error {"Invalid operands, all given registers need to be of the same width."};
		}
	}

	void BufferWriter::put_inst_mov(Registry registry, uint16_t opc, uint16_t imm, uint16_t shift) {
		uint16_t sf = registry.wide() ? 1 : 0;
		uint16_t hw = pack_shift(shift, registry.wide());

		if (!registry.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid operand, expected general purpose register."};
		}

		put_dword(sf << 31 | opc << 23 | hw << 21 | imm << 5 | registry.reg);
	}

	void BufferWriter::put_inst_orr_bitmask(Registry destination, Registry source, uint16_t n_immr_imms) {

		// destination can be SP
		if (!source.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid operand, expected source to be a general purpose register."};
		}

		if (destination.wide() != source.wide()) {
			throw std::runtime_error {"Invalid operands, all given registers need to be of the same width."};
		}

		put_inst_bitmask(0b01100100, destination, source, n_immr_imms);
	}

	void BufferWriter::put_inst_add_extended(Registry destination, Registry a, Registry b, AddType add, uint8_t imm3, bool set_flags) {

		if (b.is(Registry::STACK)) {
			throw std::runtime_error {"Invalid operands, stack register can't be used as the second source register."};
		}

		if (a.is(Registry::ZERO) || destination.is(Registry::ZERO)) {
			throw std::runtime_error {"Invalid operands, zero register can't be used as destination nor first source here."};
		}

		// fix method parameter so that we are less picky in obvious cases
		if (!b.wide() && add == AddType::SXTX) add = AddType::SXTW;
		else if (!b.wide() && add == AddType::UXTX) add = AddType::UXTW;

		uint16_t sf = destination.wide() ? 1 : 0;
		uint32_t fb = (set_flags ? 1 : 0) << 29; // S bit
		put_dword(sf << 31 | 0b0'0'01011001 << 21 | fb | b.reg << 16 | uint8_t(add) << 13 | imm3 << 10 | a.reg << 5 | destination.reg);
	}

	void BufferWriter::put_inst_adc(Registry destination, Registry a, Registry b, bool set_flags) {
		assert_register_triplet(a, b, destination);

		uint32_t sf = destination.wide() ? 1 : 0;
		uint32_t fb = (set_flags ? 1 : 0) << 29; // S bit
		put_dword(sf << 31 | 0b0'0'11010000 << 21 | fb | b.reg << 16 | 0b000000 << 10 | a.reg << 5 | destination.reg);
	}

	void BufferWriter::put_inst_count(Registry destination, Registry source, uint8_t imm1) {

		if (destination.wide() != source.wide()) {
			throw std::runtime_error {"Invalid operands, both registers need to be of the same size"};
		}

		uint32_t sf = destination.wide() ? 1 : 0;
		put_dword(sf << 31 | 0b1'0'11010110'00000'00010 << 11 | imm1 << 10 | source.reg << 5 | destination.reg);
	}

}