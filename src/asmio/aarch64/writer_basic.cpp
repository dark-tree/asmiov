#include "writer.hpp"
#include <asmio/program/linkage.hpp>

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

	void BufferWriter::put_add(Registry dst, Registry src, uint16_t imm12, bool shift12) {
		put_inst_add_imm(dst, src, imm12, shift12, false, false);
	}

	void BufferWriter::put_adds(Registry dst, Registry src, uint16_t imm12, bool shift12) {
		put_inst_add_imm(dst, src, imm12, shift12, true, false);
	}

	void BufferWriter::put_add(Registry destination, Registry a, Registry b, ShiftType shift, uint8_t imm6) {
		put_inst_add_shifted(destination, a, b, shift, imm6, false, false);
	}

	void BufferWriter::put_adds(Registry destination, Registry a, Registry b, ShiftType shift, uint8_t imm6) {
		put_inst_add_shifted(destination, a, b, shift, imm6, true, false);
	}

	void BufferWriter::put_adr(Registry destination, Label label) {
		buffer.add_linkage(label, LinkageType::AARCH64_21_5_LO_HI);
		put_dword(0b0 << 31 | 0b10000 << 24 | destination.reg);
	}

	void BufferWriter::put_adrp(Registry destination, Label label) {
		buffer.add_linkage(label, LinkageType::AARCH64_21_5_LO_HI);
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

		const auto nrs = BitPattern::try_pack(imm);

		if (nrs.ok()) {
			return put_orr(dst, dst.wide() ? XZR : WZR, nrs);
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

		if (src.is(Registry::STACK) || dst.is(Registry::STACK)) {

			// when dealing with SP zero can't be used
			if (src.is(Registry::ZERO) || dst.is(Registry::ZERO)) {
				throw std::runtime_error {"Invalid operands, zero registry can't be used int this context"};
			}

			put_add(dst, src, XZR);
			return;
		}

		put_orr(dst, src, dst.wide() ? XZR : WZR);
	}

	void BufferWriter::put_ret() {
		put_ret(LR);
	}

	void BufferWriter::put_eret() {
		put_dword(0b110'101'1'0100'11111'0000'0'0'11111'00000);
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
		buffer.add_linkage(label, LinkageType::AARCH64_19_5_ALIGNED);
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

	void BufferWriter::put_ands(Registry dst, Registry src, BitPattern pattern) {

		if (!src.is(Registry::GENERAL) || !dst.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid operand, expected general purpose register"};
		}

		put_inst_bitmask_immediate(0b11'100100, dst, src, pattern);
	}

	void BufferWriter::put_and(Registry dst, Registry src, BitPattern pattern) {

		if (!src.is(Registry::GENERAL) || !dst.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid operand, expected general purpose register"};
		}

		put_inst_bitmask_immediate(0b11'100100, dst, src, pattern);
	}

	void BufferWriter::put_ands(Registry dst, Registry a, Registry b, ShiftType shift, uint8_t imm6) {
		put_inst_shifted_register(0b1101010, 0, dst, a, b, imm6, shift);
	}

	void BufferWriter::put_and(Registry dst, Registry a, Registry b, ShiftType shift, uint8_t imm6) {
		put_inst_shifted_register(0b0001010, 0, dst, a, b, imm6, shift);
	}

	void BufferWriter::put_eor(Registry dst, Registry src, BitPattern pattern) {

		if (!src.is(Registry::GENERAL) || !dst.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid operand, expected general purpose register"};
		}

		put_inst_bitmask_immediate(0b10'100100, dst, src, pattern);
	}

	void BufferWriter::put_eor(Registry dst, Registry a, Registry b, ShiftType shift, uint8_t imm6) {
		put_inst_shifted_register(0b1001010, 0, dst, a, b, imm6, shift);
	}

	void BufferWriter::put_eon(Registry dst, Registry a, Registry b, ShiftType shift, uint8_t imm6) {
		put_inst_shifted_register(0b1001010, 1, dst, a, b, imm6, shift);
	}

	void BufferWriter::put_orr(Registry destination, Registry source, BitPattern pattern) {

		// destination can be SP
		if (!source.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid operand, expected source to be a general purpose register"};
		}

		put_inst_bitmask_immediate(0b01100100, destination, source, pattern);
	}

	void BufferWriter::put_orr(Registry dst, Registry a, Registry b, ShiftType shift, uint8_t imm6) {
		put_inst_shifted_register(0b0101010, 0, dst, a, b, imm6, shift);
	}

	void BufferWriter::put_clrex(uint8_t imm4) {
		put_dword(0b110'101'01000000110011 << 12 | (imm4 & 0b1111) << 8 | 0b010'11111);
	}

	void BufferWriter::put_svc(uint16_t imm16) {
		put_dword(0b11010100000 << 21 | imm16 << 5 | 0b00001);
	}

	void BufferWriter::put_sbc(Registry dst, Registry a, Registry b) {
		put_inst_sbc(dst, a, b, false);
	}

	void BufferWriter::put_sbcs(Registry dst, Registry a, Registry b) {
		put_inst_sbc(dst, a, b, true);
	}

	void BufferWriter::put_sub(Registry dst, Registry a, Registry b, Sizing size, uint8_t lsl3) {
		put_inst_extended_register(0b1'0'01011001, dst, a, b, size, lsl3, false);
	}

	void BufferWriter::put_subs(Registry dst, Registry a, Registry b, Sizing size, uint8_t lsl3) {
		put_inst_extended_register(0b1'0'01011001, dst, a, b, size, lsl3, true);
	}

	void BufferWriter::put_sub(Registry dst, Registry src, uint16_t imm12, bool shift_12) {
		put_inst_add_imm(dst, src, imm12, shift_12, false, true);
	}

	void BufferWriter::put_subs(Registry dst, Registry src, uint16_t imm12, bool shift_12) {
		put_inst_add_imm(dst, src, imm12, shift_12, true, true);
	}

	void BufferWriter::put_sub(Registry destination, Registry a, Registry b, ShiftType shift, uint8_t imm6) {
		put_inst_add_shifted(destination, a, b, shift, imm6, false, true);
	}

	void BufferWriter::put_subs(Registry destination, Registry a, Registry b, ShiftType shift, uint8_t imm6) {
		put_inst_add_shifted(destination, a, b, shift, imm6, true, true);
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
			throw std::runtime_error {"Invalid operands, all given registers need to be of the same width"};
		}

		uint32_t sf = dst.wide() ? 1 : 0;
		put_dword(sf << 31 | 0b0011011000 << 21 | b.reg << 16 | addend.reg << 10 | a.reg << 5 | dst.reg);
	}

	void BufferWriter::put_smaddl(Registry dst, Registry a, Registry b, Registry addend) {
		put_inst_mulopl(dst, a, b, addend, false, false);
	}

	void BufferWriter::put_umaddl(Registry dst, Registry a, Registry b, Registry addend) {
		put_inst_mulopl(dst, a, b, addend, true, false);
	}

	void BufferWriter::put_smsubl(Registry dst, Registry a, Registry b, Registry addend) {
		put_inst_mulopl(dst, a, b, addend, false, true);
	}

	void BufferWriter::put_umsubl(Registry dst, Registry a, Registry b, Registry addend) {
		put_inst_mulopl(dst, a, b, addend, true, true);
	}

	void BufferWriter::put_smnegl(Registry dst, Registry a, Registry b) {
		put_smsubl(dst, a, b, XZR);
	}

	void BufferWriter::put_umnegl(Registry dst, Registry a, Registry b) {
		put_umsubl(dst, a, b, XZR);
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

	void BufferWriter::put_sdiv(Registry dst, Registry a, Registry b) {
		put_inst_div(dst, a, b, false);
	}

	void BufferWriter::put_udiv(Registry dst, Registry a, Registry b) {
		put_inst_div(dst, a, b, true);
	}

	void BufferWriter::put_rev16(Registry dst, Registry src) {
		put_inst_rev(dst, src, 0b01);
	}

	void BufferWriter::put_rev32(Registry dst, Registry src) {
		put_inst_rev(dst, src, 0b10);
	}

	void BufferWriter::put_rev64(Registry dst, Registry src) {
		put_inst_rev(dst, src, 0b11);
	}

	void BufferWriter::put_ror(Registry dst, Registry src, Registry bits) {
		put_inst_shift_v(dst, src, bits, ShiftType::ROR);
	}

	void BufferWriter::put_lsr(Registry dst, Registry src, Registry bits) {
		put_inst_shift_v(dst, src, bits, ShiftType::LSR);
	}

	void BufferWriter::put_lsr(Registry dst, Registry src, uint16_t shift) {
		const uint32_t width = dst.size * 8;
		const uint32_t ones = width - 1; // one bit gets discarded anyway

		if (shift > ones) {
			throw std::runtime_error {"Invalid operand, can't shift by more than register width"};
		}

		// the ISA doesn't mention us needing to that but for shift=0
		// the top bit would be cut of without any shifting to cover that,
		// there were similar issues in LSL (immediate).
		if (shift == 0) {
			put_mov(dst, src);
			return;
		}

		put_ubfm(dst, src, {width, ones, shift});
	}

	void BufferWriter::put_lsl(Registry dst, Registry src, Registry bits) {
		put_inst_shift_v(dst, src, bits, ShiftType::LSL);
	}

	void BufferWriter::put_lsl(Registry dst, Registry src, uint16_t shift) {
		const uint32_t width = dst.size * 8;
		const uint32_t ones = width - 1; // one bit gets discarded anyway

		if (shift > ones) {
			throw std::runtime_error {"Invalid operand, can't shift by more than register width"};
		}

		if (shift == 0) {
			put_mov(dst, src);
			return;
		}

		// TODO we do (width - shift) here while the ISA says to use (ones - shift)
		//      but that causes the top one bit to not be copied, is that a mistake in the
		//      specification? As with length set to 64 (for shift 0) the BitPattern would be invalid
		//      we also need to check for shift=0 and encode this using a normal mov.
		//      The debugger sees this as the intended LSL alias.
		put_ubfm(dst, src, {width, width - shift, -shift % width});
	}

	void BufferWriter::put_asr(Registry dst, Registry src, Registry bits) {
		put_inst_shift_v(dst, src, bits, ShiftType::ASR);
	}

	void BufferWriter::put_asr(Registry dst, Registry src, uint16_t shift) {
		const uint32_t width = dst.size * 8;
		const uint32_t ones = width - 1; // one bit gets discarded anyway

		if (shift > ones) {
			throw std::runtime_error {"Invalid operand, can't shift by more than register width"};
		}

		// the ISA doesn't mention us needing to that but for shift=0
		// the top bit would be cut of without any shifting to cover that,
		// there were similar issues in LSL (immediate).
		if (shift == 0) {
			put_mov(dst, src);
			return;
		}

		put_sbfm(dst, src, {width, ones, shift});
	}

	void BufferWriter::put_asl(Registry dst, Registry src, Registry bits) {
		put_lsl(dst, src, bits);
	}

	void BufferWriter::put_asl(Registry dst, Registry src, uint16_t bits) {
		put_lsl(dst, src, bits);
	}

	void BufferWriter::put_ror(Registry dst, Registry src, uint8_t imm5) {
		put_extr(dst, src, src, imm5);
	}

	void BufferWriter::put_extr(Registry dst, Registry low, Registry high, uint8_t imm5) {
		assert_register_triplet(dst, low, high);
		const uint8_t max_shift = dst.wide() ? 63 : 31;

		if (imm5 > max_shift) {
			throw std::runtime_error {"Invalid operands, shift value too large for this context"};
		}

		const uint16_t sf = dst.wide() ? 1 : 0;
		put_dword(sf << 31 | 0b00100111 << 23 | sf << 22 | low.reg << 16 | imm5 << 10 | high.reg << 5 | dst.reg);
	}

	void BufferWriter::put_csel(Condition condition, Registry dst, Registry truthy, Registry falsy) {
		put_inst_csinc(condition, dst, truthy, falsy, false, false);
	}

	void BufferWriter::put_csinc(Condition condition, Registry dst, Registry truthy, Registry falsy) {
		put_inst_csinc(condition, dst, truthy, falsy, true, false);
	}

	void BufferWriter::put_cinc(Condition condition, Registry dst, Registry src) {
		put_csinc(invert(condition), dst, src, src);
	}

	void BufferWriter::put_cinc(Condition condition, Registry dst) {
		put_csinc(invert(condition), dst, dst, dst);
	}

	void BufferWriter::put_cset(Condition condition, Registry dst) {
		put_cinc(condition, dst, dst.wide() ? XZR : WZR);
	}

	void BufferWriter::put_tst(Registry a, Registry b, ShiftType shift, uint8_t lsl6) {
		put_ands(a.wide() ? XZR : WZR, a, b, shift, lsl6);
	}

	void BufferWriter::put_sbfm(Registry dst, Registry src, BitPattern pattern) {

		if (!src.is(Registry::GENERAL) || !dst.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid operand, expected general purpose register"};
		}

		put_inst_bitmask_immediate(0b00'100110, dst, src, pattern);
	}

	void BufferWriter::put_ubfm(Registry dst, Registry src, BitPattern pattern) {

		if (!src.is(Registry::GENERAL) || !dst.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid operand, expected general purpose register"};
		}

		put_inst_bitmask_immediate(0b10'100110, dst, src, pattern);
	}

	void BufferWriter::put_uxtb(Registry dst, Registry src) {
		put_ubfm(dst, src, 0xFF);
	}

	void BufferWriter::put_uxth(Registry dst, Registry src) {
		put_ubfm(dst, src, 0xFFFF);
	}

	void BufferWriter::put_bfm(Registry dst, Registry src, BitPattern pattern) {

		if (!src.is(Registry::GENERAL) || !dst.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid operand, expected general purpose register"};
		}

		put_inst_bitmask_immediate(0b01'100110, dst, src, pattern);
	}

	void BufferWriter::put_bfc(Registry dst, BitPattern pattern) {
		put_bfm(dst, dst.wide() ? XZR : WZR, pattern);
	}

	void BufferWriter::put_bic(Registry dst, Registry a, Registry b, ShiftType shift, uint8_t lsl6) {
		put_inst_bic(dst, a, b, shift, lsl6, false);
	}

	void BufferWriter::put_bics(Registry dst, Registry a, Registry b, ShiftType shift, uint8_t lsl6) {
		put_inst_bic(dst, a, b, shift, lsl6, true);
	}

	void BufferWriter::put_inst_cas(Registry ptr, Registry src, Registry cmp, Order order, uint8_t size) {
		if (!ptr.wide()) {
			throw std::runtime_error {"Invalid operand, destination register must be wide"};
		}

		if (ptr.is(Registry::ZERO)) {
			throw std::runtime_error {"Invalid operand, destination can't be the zero register"};
		}

		if (!cmp.is(Registry::GENERAL) || !src.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid operand, expected general purpose source and compare register"};
		}

		// "L" flag is used to mark acquire semantics (marked with suffix "A" in mnemonics),
		// and o0 to mark "release" (suffix "L" in mnemonics). How designed it like this??
		const uint32_t lf = is_order_acquire(order) ? (1 << 22) : 0;
		const uint32_t of = is_order_release(order) ? (1 << 15) : 0;

		put_dword(size << 30 | lf | of | 0b001000'101 << 21 | cmp.reg << 16 | 0b11111 << 10 | ptr.reg << 5 | src.reg);
	}

	void BufferWriter::put_swpb(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b00, 0b1000);
	}

	void BufferWriter::put_swph(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b01, 0b1000);
	}

	void BufferWriter::put_swp(Registry val, Registry dst, Registry src, Order order) {
		if (val.wide() != dst.wide()) {
			throw std::runtime_error {"Invalid operand, value and destination need to be of the same size"};
		}

		put_inst_ldop(val, dst, src, order, 0b10 | val.wide(), 0b1000);
	}

	void BufferWriter::put_casb(Registry dst, Registry src, Registry cmp, Order order) {
		put_inst_cas(dst, src, cmp, order, 0b00);
	}

	void BufferWriter::put_cash(Registry dst, Registry src, Registry cmp, Order order) {
		put_inst_cas(dst, src, cmp, order, 0b01);
	}

	void BufferWriter::put_cas(Registry dst, Registry src, Registry cmp, Order order) {
		if (src.wide() != cmp.wide()) {
			throw std::runtime_error {"Invalid operands, source and compare registers need to be of the same size"};
		}

		put_inst_cas(dst, src, cmp, order, 0b10 | (dst.wide() ? 1 : 0));
	}

	void BufferWriter::put_inst_ldar(Registry dst, Registry src, uint8_t size) {
		if (!src.wide()) {
			throw std::runtime_error {"Invalid operand, source register must be wide"};
		}

		if (src.is(Registry::ZERO)) {
			throw std::runtime_error {"Invalid operand, source can't be the zero register"};
		}

		if (!dst.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid operand, destination must be a general purpose register"};
		}

		put_dword(size << 30 | 0b001000'110'11111'1'11111 << 10 | src.reg << 5 | dst.reg);
	}

	void BufferWriter::put_ldarb(Registry dst, Registry src) {
		put_inst_ldar(dst, src, 0b00);
	}

	void BufferWriter::put_ldarh(Registry dst, Registry src) {
		put_inst_ldar(dst, src, 0b01);
	}

	void BufferWriter::put_ldar(Registry dst, Registry src) {
		put_inst_ldar(dst, src, 0b10 | (dst.wide() ? 1 : 0));
	}

	void BufferWriter::put_inst_ldpx(Registry r1, Registry r2, Registry src, int64_t offset, MemoryOperation op, uint32_t size, bool load, uint32_t opc) {
		if (!src.wide()) {
			throw std::runtime_error {"Invalid operand, source register must be wide"};
		}

		if (src.is(Registry::ZERO)) {
			throw std::runtime_error {"Invalid operand, source can't be the zero register"};
		}

		if (r1.wide() != r2.wide()) {
			throw std::runtime_error {"Invalid operands, both destination registers need to be of the same size"};
		}

		if (!r1.is(Registry::GENERAL) || !r2.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid operands, both destination registers need to be general purpose"};
		}

		int32_t imm7 = offset / size;

		if (offset % size) {
			throw std::runtime_error {"Invalid operand, immediate value not divisible by operand size"};
		}

		if (imm7 < -64 || imm7 > 63) {
			throw std::runtime_error {"Invalid operand, immediate value out of range"};
		}

		// TODO maybe we can find some common bit pattern for this enum?
		uint32_t bits = 0;
		if (op == POST) bits = 0b01;
		else if (op == PRE) bits = 0b11;
		else if (op == OFFSET) bits = 0b10;

		put_dword(opc << 30 | 0b101 << 27 | bits << 23 | load << 22 | (imm7 & 0x7f) << 15 | r2.reg << 10 | src.reg << 5 | r1.reg);
	}

	void BufferWriter::put_ldp(Registry r1, Registry r2, Registry src, int64_t offset) {
		put_inst_ldpx(r1, r2, src, offset, OFFSET, r1.size, true, r1.wide() << 1);
	}

	void BufferWriter::put_ildp(Registry r1, Registry r2, Registry src, int64_t offset) {
		put_inst_ldpx(r1, r2, src, offset, PRE, r1.size, true, r1.wide() << 1);
	}

	void BufferWriter::put_ldpi(Registry r1, Registry r2, Registry src, int64_t offset) {
		put_inst_ldpx(r1, r2, src, offset, POST, r1.size, true, r1.wide() << 1);
	}

	void BufferWriter::put_ldpsw(Registry r1, Registry r2, Registry src, int64_t offset) {
		if (!r1.wide()) {
			throw std::runtime_error {"Invalid operand, expected qword destination registers"};
		}

		put_inst_ldpx(r1, r2, src, offset, OFFSET, DWORD, true, 1);
	}

	void BufferWriter::put_ildpsw(Registry r1, Registry r2, Registry src, int64_t offset) {
		if (!r1.wide()) {
			throw std::runtime_error {"Invalid operand, expected qword destination registers"};
		}

		put_inst_ldpx(r1, r2, src, offset, PRE, DWORD, true, 1);
	}

	void BufferWriter::put_ldpswi(Registry r1, Registry r2, Registry src, int64_t offset) {
		if (!r1.wide()) {
			throw std::runtime_error {"Invalid operand, expected qword destination registers"};
		}

		put_inst_ldpx(r1, r2, src, offset, POST, DWORD, true, 1);
	}

	void BufferWriter::put_ldnp(Registry r1, Registry r2, Registry src, int64_t offset) {
		// FIXME this cast is an ugly hack, we use this to get bits == 0 in put_inst_ldpx()
		put_inst_ldpx(r1, r2, src, offset, static_cast<MemoryOperation>(-1), r1.size, true, r1.wide() << 1);
	}

	void BufferWriter::put_stp(Registry r1, Registry r2, Registry src, int64_t offset) {
		put_inst_ldpx(r1, r2, src, offset, OFFSET, r1.size, false, r1.wide() << 1);
	}

	void BufferWriter::put_istp(Registry r1, Registry r2, Registry src, int64_t offset) {
		put_inst_ldpx(r1, r2, src, offset, PRE, r1.size, false, r1.wide() << 1);
	}

	void BufferWriter::put_stpi(Registry r1, Registry r2, Registry src, int64_t offset) {
		put_inst_ldpx(r1, r2, src, offset, POST, r1.size, false, r1.wide() << 1);
	}

	void BufferWriter::put_stnp(Registry r1, Registry r2, Registry src, int64_t offset) {
		// FIXME this cast is an ugly hack, we use this to get bits == 0 in put_inst_ldpx()
		put_inst_ldpx(r1, r2, src, offset, static_cast<MemoryOperation>(-1), r1.size, false, r1.wide() << 1);
	}

	void BufferWriter::put_ccmp(Condition condition, Condition flags, Registry val, uint8_t imm5) {
		put_dword(val.wide() << 31 | 0b11'11010010 << 21 | (imm5 & 0b11111) << 16 | uint32_t(condition) << 12 | 0b10 << 10 | val.reg << 5 | uint32_t(flags));
	}

	void BufferWriter::put_ccmn(Condition condition, Condition flags, Registry val, uint8_t imm5) {
		put_dword(val.wide() << 31 | 0b01'11010010 << 21 | (imm5 & 0b11111) << 16 | uint32_t(condition) << 12 | 0b10 << 10 | val.reg << 5 | uint32_t(flags));
	}

	void BufferWriter::put_ccmp(Condition condition, Condition flags, Registry val, Registry second) {
		if (val.size != second.size) {
			throw std::runtime_error {"Invalid operands, both registers need to be of the same size"};
		}

		put_dword(val.wide() << 31 | 0b11'11010010 << 21 | second.reg << 16 | uint32_t(condition) << 12 | 0b00 << 10 | val.reg << 5 | uint32_t(flags));
	}

	void BufferWriter::put_ccmn(Condition condition, Condition flags, Registry val, Registry second) {
		if (val.size != second.size) {
			throw std::runtime_error {"Invalid operands, both registers need to be of the same size"};
		}

		put_dword(val.wide() << 31 | 0b01'11010010 << 21 | second.reg << 16 | uint32_t(condition) << 12 | 0b00 << 10 | val.reg << 5 | uint32_t(flags));
	}

	void BufferWriter::put_csinv(Condition condition, Registry dst, Registry truthy, Registry falsy) {
		put_inst_csinc(condition, dst, truthy, falsy, false, true);
	}

	void BufferWriter::put_cinv(Condition condition, Registry dst, Registry src) {
		put_inst_csinc(invert(condition), dst, src, src, false, true);
	}

	void BufferWriter::put_csetm(Condition condition, Registry dst) {
		Registry zero = dst.wide() ? XZR : WZR;
		put_inst_csinc(invert(condition), dst, zero, zero, false, true);
	}

	void BufferWriter::put_csneg(Condition condition, Registry dst, Registry truthy, Registry falsy) {
		put_inst_csinc(condition, dst, truthy, falsy, true, true);
	}

	void BufferWriter::put_cneg(Condition condition, Registry dst, Registry src) {
		put_inst_csinc(condition, dst, src, src, true, true);
	}

	void BufferWriter::put_cmn(Registry src, uint16_t imm12, bool shift_12) {
		Registry zero = src.wide() ? XZR : WZR;
		put_adds(zero, src, imm12, shift_12);
	}

	void BufferWriter::put_cmn(Registry a, Registry b, ShiftType shift, uint8_t imm6) {
		Registry zero = a.wide() ? XZR : WZR;
		put_adds(zero, a, b, shift, imm6);
	}

	void BufferWriter::put_cmp(Registry src, uint16_t imm12, bool shift_12) {
		Registry zero = src.wide() ? XZR : WZR;
		put_subs(zero, src, imm12, shift_12);
	}

	void BufferWriter::put_cmp(Registry a, Registry b, ShiftType shift, uint8_t imm6) {
		Registry zero = a.wide() ? XZR : WZR;
		put_subs(zero, a, b, shift, imm6);
	}

	void BufferWriter::put_ldxp(Registry r1, Registry r2, Registry src) {
		put_inst_ldstx(r1, r2, XZR, src, 0b10 | r1.wide(), true, false, true);
	}

	void BufferWriter::put_ldaxp(Registry r1, Registry r2, Registry src) {
		put_inst_ldstx(r1, r2, XZR, src, 0b10 | r1.wide(), true, true, true);
	}

	void BufferWriter::put_ldaxr(Registry dst, Registry src) {
		put_inst_ldstx(dst, XZR, XZR, src, 0b10 | dst.wide(), true, true, false);
	}

	void BufferWriter::put_ldaxrh(Registry dst, Registry src) {
		put_inst_ldstx(dst, XZR, XZR, src, 0b01, true, true, false);
	}

	void BufferWriter::put_ldaxrb(Registry dst, Registry src) {
		put_inst_ldstx(dst, XZR, XZR, src, 0b00, true, true, false);
	}

	void BufferWriter::put_ldxr(Registry dst, Registry src) {
		put_inst_ldstx(dst, XZR, XZR, src, 0b10 | dst.wide(), true, false, false);
	}

	void BufferWriter::put_ldxrh(Registry dst, Registry src) {
		put_inst_ldstx(dst, XZR, XZR, src, 0b01, true, false, false);
	}

	void BufferWriter::put_ldxrb(Registry dst, Registry src) {
		put_inst_ldstx(dst, XZR, XZR, src, 0b00, true, false, false);
	}

	void BufferWriter::put_stlxr(Registry status, Registry dst, Registry src) {
		put_inst_ldstx(dst, XZR, status, src, 0b10 | dst.wide(), false, true, false);
	}

	void BufferWriter::put_stlxrh(Registry status, Registry dst, Registry src) {
		put_inst_ldstx(dst, XZR, status, src, 0b01, false, true, false);
	}

	void BufferWriter::put_stlxrb(Registry status, Registry dst, Registry src) {
		put_inst_ldstx(dst, XZR, status, src, 0b00, false, true, false);
	}

	void BufferWriter::put_stxr(Registry status, Registry dst, Registry src) {
		put_inst_ldstx(dst, XZR, status, src, 0b10 | dst.wide(), false, false, false);
	}

	void BufferWriter::put_stxrh(Registry status, Registry dst, Registry src) {
		put_inst_ldstx(dst, XZR, status, src, 0b01, false, false, false);
	}

	void BufferWriter::put_stxrb(Registry status, Registry dst, Registry src) {
		put_inst_ldstx(dst, XZR, status, src, 0b00, false, false, false);
	}

	void BufferWriter::put_ldtr(Registry dst, Registry src, int16_t offset) {
		put_inst_ldst_simm9(dst, src, offset, 0b10 | dst.wide(), true, 0b10);
	}

	void BufferWriter::put_ldtrh(Registry dst, Registry src, int16_t offset) {
		put_inst_ldst_simm9(dst, src, offset, 0b01, true, 0b10);
	}

	void BufferWriter::put_ldtrb(Registry dst, Registry src, int16_t offset) {
		put_inst_ldst_simm9(dst, src, offset, 0b00, true, 0b10);
	}

	void BufferWriter::put_sttr(Registry dst, Registry src, int16_t offset) {
		put_inst_ldst_simm9(dst, src, offset, 0b10 | dst.wide(), false, 0b10);
	}

	void BufferWriter::put_sttrh(Registry dst, Registry src, int16_t offset) {
		put_inst_ldst_simm9(dst, src, offset, 0b01, false, 0b10);
	}

	void BufferWriter::put_sttrb(Registry dst, Registry src, int16_t offset) {
		put_inst_ldst_simm9(dst, src, offset, 0b00, false, 0b10);
	}

	void BufferWriter::put_ldur(Registry dst, Registry src, int16_t offset) {
		put_inst_ldst_simm9(dst, src, offset, 0b10 | dst.wide(), true, 0b00);
	}

	void BufferWriter::put_ldurh(Registry dst, Registry src, int16_t offset) {
		put_inst_ldst_simm9(dst, src, offset, 0b01, true, 0b00);
	}

	void BufferWriter::put_ldurb(Registry dst, Registry src, int16_t offset) {
		put_inst_ldst_simm9(dst, src, offset, 0b00, true, 0b00);
	}

	void BufferWriter::put_stur(Registry dst, Registry src, int16_t offset) {
		put_inst_ldst_simm9(dst, src, offset, 0b10 | dst.wide(), false, 0b00);
	}

	void BufferWriter::put_sturh(Registry dst, Registry src, int16_t offset) {
		put_inst_ldst_simm9(dst, src, offset, 0b01, false, 0b00);
	}

	void BufferWriter::put_sturb(Registry dst, Registry src, int16_t offset) {
		put_inst_ldst_simm9(dst, src, offset, 0b00, false, 0b00);
	}

	void BufferWriter::put_stlxp(Registry status, Registry r1, Registry r2, Registry src) {
		put_inst_ldstx(r1, r2, status, src, 0b10 | r1.wide(), false, true, true);
	}

	void BufferWriter::put_stxp(Registry status, Registry r1, Registry r2, Registry src) {
		put_inst_ldstx(r2, r2, status, src, 0b10 | r1.wide(), false, false, true);
	}

	void BufferWriter::put_inst_ldop(Registry val, Registry dst, Registry src, Order order, uint8_t size, uint32_t opc) {
		if (!src.wide()) {
			throw std::runtime_error {"Invalid operand, source and destination register must be wide"};
		}

		if (src.is(Registry::ZERO)) {
			throw std::runtime_error {"Invalid operand, source can't be the zero register"};
		}

		if (!val.is(Registry::GENERAL) || !dst.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid operand, value and destination must be general purpose registers"};
		}

		uint32_t a = is_order_acquire(order) ? (1 << 23) : 0;
		uint32_t r = is_order_release(order) ? (1 << 22) : 0;
		put_dword(size << 30 | 0b111000'00'1 << 21 | val.reg << 16 | opc << 12 | src.reg << 5 | dst.reg | a | r);
	}

	void BufferWriter::put_ldaddb(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b00, 0b000);
	}

	void BufferWriter::put_ldaddh(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b01, 0b000);
	}

	void BufferWriter::put_ldadd(Registry val, Registry dst, Registry src, Order order) {
		if (val.wide() != dst.wide()) {
			throw std::runtime_error {"Invalid operand, value and destination need to be of the same size"};
		}

		put_inst_ldop(val, dst, src, order, 0b10 | val.wide(), 0b000);
	}

	void BufferWriter::put_ldclrb(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b00, 0b001);
	}

	void BufferWriter::put_ldclrh(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b01, 0b001);
	}

	void BufferWriter::put_ldclr(Registry val, Registry dst, Registry src, Order order) {
		if (val.wide() != dst.wide()) {
			throw std::runtime_error {"Invalid operand, value and destination need to be of the same size"};
		}

		put_inst_ldop(val, dst, src, order, 0b10 | val.wide(), 0b001);
	}

	void BufferWriter::put_ldeorb(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b00, 0b010);
	}

	void BufferWriter::put_ldeorh(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b01, 0b010);
	}

	void BufferWriter::put_ldeor(Registry val, Registry dst, Registry src, Order order) {
		if (val.wide() != dst.wide()) {
			throw std::runtime_error {"Invalid operand, value and destination need to be of the same size"};
		}

		put_inst_ldop(val, dst, src, order, 0b10 | val.wide(), 0b010);
	}

	void BufferWriter::put_ldsetb(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b00, 0b011);
	}

	void BufferWriter::put_ldseth(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b01, 0b011);
	}

	void BufferWriter::put_ldset(Registry val, Registry dst, Registry src, Order order) {
		if (val.wide() != dst.wide()) {
			throw std::runtime_error {"Invalid operand, value and destination need to be of the same size"};
		}

		put_inst_ldop(val, dst, src, order, 0b10 | val.wide(), 0b011);
	}

	void BufferWriter::put_ldsmaxb(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b00, 0b100);
	}

	void BufferWriter::put_ldsmaxh(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b01, 0b100);
	}

	void BufferWriter::put_ldsmax(Registry val, Registry dst, Registry src, Order order) {
		if (val.wide() != dst.wide()) {
			throw std::runtime_error {"Invalid operand, value and destination need to be of the same size"};
		}

		put_inst_ldop(val, dst, src, order, 0b10 | val.wide(), 0b100);
	}

	void BufferWriter::put_ldumaxb(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b00, 0b110);
	}

	void BufferWriter::put_ldumaxh(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b01, 0b110);
	}

	void BufferWriter::put_ldumax(Registry val, Registry dst, Registry src, Order order) {
		if (val.wide() != dst.wide()) {
			throw std::runtime_error {"Invalid operand, value and destination need to be of the same size"};
		}

		put_inst_ldop(val, dst, src, order, 0b10 | val.wide(), 0b110);
	}

	void BufferWriter::put_ldsminb(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b00, 0b101);
	}

	void BufferWriter::put_ldsminh(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b01, 0b101);
	}

	void BufferWriter::put_ldsmin(Registry val, Registry dst, Registry src, Order order) {
		if (val.wide() != dst.wide()) {
			throw std::runtime_error {"Invalid operand, value and destination need to be of the same size"};
		}

		put_inst_ldop(val, dst, src, order, 0b10 | val.wide(), 0b101);
	}

	void BufferWriter::put_lduminb(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b00, 0b111);
	}

	void BufferWriter::put_lduminh(Registry val, Registry dst, Registry src, Order order) {
		put_inst_ldop(val, dst, src, order, 0b01, 0b111);
	}

	void BufferWriter::put_ldumin(Registry val, Registry dst, Registry src, Order order) {
		if (val.wide() != dst.wide()) {
			throw std::runtime_error {"Invalid operand, value and destination need to be of the same size"};
		}

		put_inst_ldop(val, dst, src, order, 0b10 | val.wide(), 0b111);
	}

	void BufferWriter::put_hint(uint8_t imm7) {
		put_dword(0b1101010100'0'00'011'0010 << 12 | (0b1111'111 & imm7) << 5 | 0b11111);
	}

	void BufferWriter::put_hlt(uint16_t imm16) {
		put_dword(0b11010100'010 << 21 | imm16 << 5 | 0b000'00);
	}

	void BufferWriter::put_hvc(uint16_t imm16) {
		put_dword(0b11010100'000 << 21 | imm16 << 5 | 0b000'10);
	}

	void BufferWriter::put_smc(uint16_t imm16) {
		put_dword(0b11010100'000 << 21 | imm16 << 5 | 0b000'11);
	}

	void BufferWriter::put_brk(uint16_t imm) {
		put_dword(0b11010100'001 << 21 | imm << 5 | 0b00000);
	}

	void BufferWriter::put_isb() {
		put_dword(0b1101010100'0'00'011'0011 << 12 | 0b1111 << 8 | 0b1'10'11111);
	}

	void BufferWriter::put_nop() {
		put_hint(0b0000'000);
	}

	void BufferWriter::put_yield() {
		put_hint(0b0000'001);
	}

	void BufferWriter::put_wfe() {
		put_hint(0b0000'010);
	}

	void BufferWriter::put_wfi() {
		put_hint(0b0000'011);
	}

	void BufferWriter::put_sev() {
		put_hint(0b0000'100);
	}

	void BufferWriter::put_sevl() {
		put_hint(0b0000'101);
	}

	void BufferWriter::put_esb() {
		put_hint(0b0010'000);
	}

	void BufferWriter::put_psb() {
		put_hint(0b0010'001);
	}

}
