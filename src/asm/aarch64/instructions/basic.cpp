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

	void BufferWriter::put_add(Registry dst, Registry a, Registry b, Sizing size, uint8_t lsl3) {
		put_inst_extended_register(0b0'0'01011001, dst, a, b, size, lsl3, false);
	}

	void BufferWriter::put_adds(Registry dst, Registry a, Registry b, Sizing size, uint8_t lsl3) {
		put_inst_extended_register(0b0'0'01011001, dst, a, b, size, lsl3, true);
	}

	void BufferWriter::put_adr(Registry destination, Label label) {
		buffer.add_linkage(label, 0, link_21_5_lo_hi);
		put_dword(0b0 << 31 | 0b10000 << 24 | destination.reg);
	}

	void BufferWriter::put_adrp(Registry destination, Label label) {
		buffer.add_linkage(label, 0, link_21_5_lo_hi);
		put_dword(0b1 << 31 | 0b10000 << 24 | destination.reg);
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

	void BufferWriter::put_brk(uint16_t imm) {
		put_dword(0b11010100'001 << 21 | imm << 5 | 0b00000);
	}

	void BufferWriter::put_ret(Registry registry) {

		if (!registry.wide()) {
			throw std::runtime_error {"Invalid operand, non-qword register can't be used here"};
		}

		if (!registry.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid operand, expected general purpose register"};
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

	void BufferWriter::put_clz(Registry dst, Registry src) {
		put_inst_count(dst, src, 0);
	}

	void BufferWriter::put_cls(Registry dst, Registry src) {
		put_inst_count(dst, src, 1);
	}

	void BufferWriter::put_ldr(Registry registry, Label label) {
		uint16_t sf = registry.wide() ? 1 : 0;
		buffer.add_linkage(label, 0, link_19_5_aligned);
		put_dword(sf << 30 | 0b011000 << 24 | registry.reg);
	}

	void BufferWriter::put_ldri(Registry dst, Registry base, int64_t offset, Sizing sizing) {
		put_inst_ldst(dst, base, offset, sizing, POST, LOAD);
	}

	void BufferWriter::put_ildr(Registry dst, Registry base, int64_t offset, Sizing sizing) {
		put_inst_ldst(dst, base, offset, sizing, PRE, LOAD);
	}

	void BufferWriter::put_ldr(Registry dst, Registry base, uint64_t offset, Sizing sizing) {
		put_inst_ldst(dst, base, std::bit_cast<int64_t>(offset), sizing, OFFSET, LOAD);
	}

	void BufferWriter::put_stri(Registry dst, Registry base, int64_t offset, Sizing sizing) {
		put_inst_ldst(dst, base, offset, sizing, POST, STORE);
	}

	void BufferWriter::put_istr(Registry dst, Registry base, int64_t offset, Sizing sizing) {
		put_inst_ldst(dst, base, offset, sizing, PRE, STORE);
	}

	void BufferWriter::put_str(Registry dst, Registry base, uint64_t offset, Sizing sizing) {
		put_inst_ldst(dst, base, std::bit_cast<int64_t>(offset), sizing, OFFSET, STORE);
	}

	void BufferWriter::put_and(Registry dst, Registry a, Registry b, ShiftType shift, uint8_t imm6) {
		put_inst_shifted_register(0b0001010, dst, a, b, imm6, shift);
	}

	void BufferWriter::put_eor(Registry dst, Registry a, Registry b, ShiftType shift, uint8_t imm6) {
		put_inst_shifted_register(0b1001010, dst, a, b, imm6, shift);
	}

	void BufferWriter::put_orr(Registry dst, Registry a, Registry b, ShiftType shift, uint8_t imm6) {
		put_inst_shifted_register(0b0101010, dst, a, b, imm6, shift);
	}

	void BufferWriter::put_svc(uint16_t imm16) {
		put_dword(0b11010100000 << 21 | imm16 << 5 | 0b00001);
	}

	void BufferWriter::put_sub(Registry dst, Registry a, Registry b, Sizing size, uint8_t lsl3) {
		put_inst_extended_register(0b1'0'01011001, dst, a, b, size, lsl3, false);
	}

	void BufferWriter::put_subs(Registry dst, Registry a, Registry b, Sizing size, uint8_t lsl3) {
		put_inst_extended_register(0b1'0'01011001, dst, a, b, size, lsl3, true);
	}

	void BufferWriter::put_cmp(Registry a, Registry b, Sizing size, uint8_t lsl3) {
		put_subs(a.wide() ? XZR : WZR, a, b, size, lsl3);
	}

	void BufferWriter::put_cmn(Registry a, Registry b, Sizing size, uint8_t lsl3) {
		put_adds(a.wide() ? XZR : WZR, a, b, size, lsl3);
	}

	void BufferWriter::put_madd(Registry dst, Registry a, Registry b, Registry addend) {
		assert_register_triplet(a, b, dst);

		// we have four register so the last one needs to be checked manually
		if (dst.wide() != addend.wide()) {
			throw std::runtime_error {"Invalid operands, all given registers need to be of the same width."};
		}

		uint32_t sf = dst.wide() ? 1 : 0;
		put_dword(sf << 31 | 0b0011011000 << 21 | b.reg << 16 | addend.reg << 10 | a.reg << 5 | dst.reg);
	}

	void BufferWriter::put_smaddl(Registry dst, Registry a, Registry b, Registry addend) {
		put_inst_maddl(dst, a, b, addend, false);
	}

	void BufferWriter::put_umaddl(Registry dst, Registry a, Registry b, Registry addend) {
		put_inst_maddl(dst, a, b, addend, true);
	}

	void BufferWriter::put_mul(Registry dst, Registry a, Registry b) {
		put_madd(dst, a, b, dst.wide() ? XZR : WZR);
	}

	void BufferWriter::put_smul(Registry dst, Registry a, Registry b) {
		put_smaddl(dst, a, b, XZR);
	}

	void BufferWriter::put_umul(Registry dst, Registry a, Registry b) {
		put_umaddl(dst, a, b, XZR);
	}

	void BufferWriter::put_smulh(Registry dst, Registry a, Registry b) {
		put_inst_mulh(dst, a, b, false);
	}

	void BufferWriter::put_umulh(Registry dst, Registry a, Registry b) {
		put_inst_mulh(dst, a, b, true);
	}

}
