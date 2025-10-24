#include "../writer.hpp"

namespace asmio::arm {

	/*
	 * class BufferWriter
	 */

	void BufferWriter::put_adc(Registry dst, Registry a, Registry b) {
		put_inst_adc(dst, a, b, false);
	}

	void BufferWriter::put_adcs(Registry dst, Registry a, Registry b) {
		put_inst_adc(dst, a, b, true);
	}

	void BufferWriter::put_add(Registry dst, Registry a, Registry b, AddType size, uint8_t lsl3) {
		put_inst_add_extended(dst, a, b, size, lsl3, false);
	}

	void BufferWriter::put_adds(Registry dst, Registry a, Registry b, AddType size, uint8_t lsl3) {
		put_inst_add_extended(dst, a, b, size, lsl3, true);
	}

	void BufferWriter::put_adr(Registry destination, uint64_t imm22) {

		if (imm22 > util::bit_fill<uint64_t>(22)) {
			throw std::runtime_error {"Invalid operands, offset can be at most 22 bits long."};
		}

		const uint32_t immlo = imm22 & 0b11;
		const uint32_t immhi = imm22 >> 2;

		put_dword(0b0 << 31 | immlo << 29 | 0b10000 << 24 | immhi << 5 | destination.reg);
	}

	void BufferWriter::put_adrp(Registry destination, uint64_t imm22) {

		if (imm22 > util::bit_fill<uint64_t>(22)) {
			throw std::runtime_error {"Invalid operands, offset can be at most 22 bits long."};
		}

		const uint32_t immlo = imm22 & 0b11;
		const uint32_t immhi = imm22 >> 2;

		put_dword(0b1 << 31 | immlo << 29 | 0b10000 << 24 | immhi << 5 | destination.reg);
	}

	void BufferWriter::put_movz(Registry registry, uint16_t imm, uint16_t shift) {
		put_inst_mov(registry, 0b10100101, imm, shift);
	}

	void BufferWriter::put_movk(Registry registry, uint16_t imm, uint16_t shift) {
		put_inst_mov(registry, 0b11100101, imm, shift);
	}

	void BufferWriter::put_movn(Registry registry, uint16_t imm, uint16_t shift) {
		put_inst_mov(registry, 0b00100101, imm, shift);
	}

	void BufferWriter::put_mov(Registry dst, uint64_t imm) {

		if (dst.is(Registry::ZERO)) {
			return; // do nothing
		}

		if (imm <= UINT16_MAX) {
			put_movz(dst, imm);
			return;
		}

		const uint64_t inv = ~imm;

		if (inv <= UINT16_MAX) {
			put_movn(dst, inv);
			return;
		}

		const auto nrs = compute_immediate_bitmask(imm, dst.wide());

		if (nrs.has_value()) {
			put_inst_orr_bitmask(dst, dst.wide() ? XZR : WZR, nrs.value());
			return;
		}

		const size_t length = dst.wide() ? 64 : 32;

		// TODO this can be made better by using movn/movz strategically here
		put_movz(dst, imm & UINT16_MAX);

		for (size_t i = 16; i < length; i += 16) {
			imm >>= 16;
			uint16_t part = imm & UINT16_MAX;

			if (part) {
				put_movk(dst, part, i);
			}
		}

	}

	void BufferWriter::put_mov(Registry dst, Registry src) {
		put_inst_orr(dst, src, dst.wide() ? XZR : WZR);
	}

	void BufferWriter::put_nop() {
		put_dword(0b1101010100'0'00'011'0010'0000'000'11111);
	}

	void BufferWriter::put_ret() {
		put_ret(LR);
	}

	void BufferWriter::put_ret(Registry registry) {

		if (!registry.wide()) {
			throw std::runtime_error {"Invalid operand, non-qword register can't be used here"};
		}

		if (registry.is(Registry::GENERAL)) {

		}

		put_dword(0b1101011001011111000000'00000'00000 | registry.reg << 5);
	}

	void BufferWriter::put_rbit(Registry dst, Registry src) {

		if (dst.wide() != src.wide()) {
			throw std::runtime_error {"Invalid operands, both registers need to be of the same size"};
		}

		uint16_t sf = dst.wide() ? 1 : 0;
		put_dword(sf << 31 | 0b1011010110 << 21 | src.reg << 5 | dst.reg);
	}

}
