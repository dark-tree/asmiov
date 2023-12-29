#pragma once

#include "writer.hpp"

namespace asmio::x86 {

	/// Move
	void BufferWriter::put_mov(Location dst, Location src) {

		// verify operand size
		if (src.is_memreg() && dst.is_memreg()) {
			if (src.size != dst.size) {
				throw std::runtime_error{"Operands must have equal size!"};
			}
		}

		// short form, VAL to REG
		if (src.is_immediate() && dst.is_simple()) {
			Registry dst_reg = dst.base;

			if (dst.base.size == WORD) {
				put_inst_16bit_operand_mark();
			}

			put_byte((0b1011 << 4) | (dst_reg.is_wide() << 3) | dst_reg.reg);
			put_inst_label_imm(src, dst.base.size);
			return;
		}

		// handle REG/MEM to REG
		if (dst.is_simple() && src.is_memreg()) {
			put_inst_mov(src, dst, true);

			return;
		}

		// handle REG/VAL to REG/MEM
		if ((src.is_immediate() || src.is_simple()) && dst.is_memreg()) {
			put_inst_mov(dst, src, src.is_immediate());

			return;
		}

		throw std::runtime_error{"Invalid operands!"};
	}

	/// Move with Sign Extension
	void BufferWriter::put_movsx(Location dst, Location src) {
		put_inst_movx(0b101111, dst, src);
	}

	/// Move with Zero Extension
	void BufferWriter::put_movzx(Location dst, Location src) {
		put_inst_movx(0b101101, dst, src);
	}

	/// Load Effective Address
	void BufferWriter::put_lea(Location dst, Location src) {

		// lea deals with addresses, addresses use 32 bits,
		// so this is quite a logical limitation
		if (dst.base.size != DWORD) {
			throw std::runtime_error{"Invalid operands, non-dword target register can't be used here!"};
		}

		// handle EXP to REG
		if (dst.is_simple() && !src.reference) {
			put_inst_std(0b10001101, src, dst.base.reg);
			return;
		}

		if (src.reference) {
			throw std::runtime_error{"Invalid operands, reference can't be used here!"};
		}

		throw std::runtime_error{"Invalid operands!"};
	}

	/// Exchange
	void BufferWriter::put_xchg(Location dst, Location src) {

		if (dst.is_simple() && !src.base.is(UNSET)) {
			put_inst_std(0b100001, src, dst.base.reg, true, src.base.is_wide());
			return;
		}

		if (!dst.base.is(UNSET) && src.is_simple()) {
			put_inst_std(0b100001, dst, src.base.reg, true, dst.base.is_wide());
			return;
		}

		throw std::runtime_error{"Invalid operands!"};
	}

	/// Push
	void BufferWriter::put_push(Location src) {

		// handle immediate data
		if (src.is_immediate()) {
			uint8_t imm_len = min_bytes(src.offset);

			if (imm_len == BYTE) {
				put_byte(0b01101010);
				put_inst_label_imm(src, BYTE);
			} else {
				put_byte(0b01101000);
				put_inst_label_imm(src, DWORD);
			}

			return;
		}

		// for some reason push & pop don't handle the wide flag,
		// so we can only accept wide registers
		if (src.size == BYTE) {
			throw std::runtime_error{"Invalid operands, byte register can't be used here!"};
		}

		// short-form
		if (src.is_simple()) {

			if (src.base.size == WORD) {
				put_inst_16bit_operand_mark();
			}

			put_byte((0b01010 << 3) | src.base.reg);
			return;
		}

		if (!src.base.is(UNSET)) {
			put_inst_std(0b11111111, 0b110, src.base.reg);
			return;
		}

		throw std::runtime_error{"Invalid operands!"};
	}

	/// Pop
	void BufferWriter::put_pop(Location src) {

		// for some reason push & pop don't handle the wide flag,
		// so we can only accept wide registers
		if (!src.base.is_wide()) {
			throw std::runtime_error{"Invalid operands, byte register can't be used here!"};
		}

		// short-form
		if (src.is_simple()) {

			if (src.base.size == WORD) {
				put_inst_16bit_operand_mark();
			}

			put_byte((0b01011 << 3) | src.base.reg);
			return;
		}

		if (!src.base.is(UNSET)) {
			put_inst_std(0b100011, 0b000, src.base.reg, true, src.base.is_wide());
			return;
		}

		throw std::runtime_error{"Invalid operands!"};
	}

	/// Increment
	void BufferWriter::put_inc(Location dst) {

		// short-form
		if (dst.is_simple() && dst.base.is_wide()) {
			Registry dst_reg = dst.base;

			if (dst.base.size == WORD) {
				put_inst_16bit_operand_mark();
			}

			put_byte((0b01000 << 3) | dst_reg.reg);
			return;
		}

		if (!dst.base.is(UNSET)) {
			put_inst_std(0b111111, dst, 0b000, true, dst.base.is_wide());
			return;
		}

		throw std::runtime_error{"Invalid operands!"};
	}

	/// Decrement
	void BufferWriter::put_dec(Location dst) {

		// short-form
		if (dst.is_simple() && dst.base.is_wide()) {

			if (dst.base.size == WORD) {
				put_inst_16bit_operand_mark();
			}

			put_byte((0b01001 << 3) | dst.base.reg);
			return;
		}

		if (dst.is_memreg()) {
			put_inst_std(0b111111, dst, 0b001, true, dst.is_wide());
			return;
		}

		throw std::runtime_error{"Invalid operands!"};
	}

	/// Negate
	void BufferWriter::put_neg(Location dst) {
		if (!dst.base.is(UNSET)) {
			put_inst_std(0b111101, dst, 0b011, true, dst.base.is_wide());
			return;
		}

		throw std::runtime_error{"Invalid operands!"};
	}

	/// Add
	void BufferWriter::put_add(Location dst, Location src) {
		put_inst_tuple(dst, src, 0b000000, 0b000);
	}

	/// Add with carry
	void BufferWriter::put_adc(Location dst, Location src) {
		put_inst_tuple(dst, src, 0b000100, 0b010);
	}

	/// Subtract
	void BufferWriter::put_sub(Location dst, Location src) {
		put_inst_tuple(dst, src, 0b001010, 0b101);
	}

	/// Subtract with borrow
	void BufferWriter::put_sbb(Location dst, Location src) {
		put_inst_tuple(dst, src, 0b000110, 0b011);
	}

	/// Compare
	void BufferWriter::put_cmp(Location dst, Location src) {
		put_inst_tuple(dst, src, 0b001110, 0b111);
	}

	/// Binary And
	void BufferWriter::put_and(Location dst, Location src) {
		put_inst_tuple(dst, src, 0b001000, 0b110);
	}

	/// Binary Or
	void BufferWriter::put_or(Location dst, Location src) {
		put_inst_tuple(dst, src, 0b000010, 0b001);
	}

	/// Binary Xor
	void BufferWriter::put_xor(Location dst, Location src) {
		put_inst_tuple(dst, src, 0b001100, 0b010);
	}

	/// Bit Test
	void BufferWriter::put_bt(Location dst, Location src) {
		put_inst_btx(dst, src, 0b101000, 0b100);
	}

	/// Bit Test and Set
	void BufferWriter::put_bts(Location dst, Location src) {
		put_inst_btx(dst, src, 0b101010, 0b101);
	}

	/// Bit Test and Reset
	void BufferWriter::put_btr(Location dst, Location src) {
		put_inst_btx(dst, src, 0b101100, 0b110);
	}

	/// Bit Test and Complement
	void BufferWriter::put_btc(Location dst, Location src) {
		put_inst_btx(dst, src, 0b101110, 0b111);
	}

	/// Multiply (Unsigned)
	void BufferWriter::put_mul(Location dst) {
		if (dst.is_simple() || dst.reference) {
			put_inst_std(0b111101, dst, 0b100, true, dst.base.is_wide());
			return;
		}

		throw std::runtime_error{"Invalid operands!"};
	}

	/// Integer multiply (Signed)
	void BufferWriter::put_imul(Location dst, Location src) {

		// TODO: This requires a change to the size system to work
		// short form
		// if (dst.is_simple() && src.is_memreg() && dst.base.name == Registry::Name::A) {
		// 	put_inst_std(0b111101, src, 0b101, true, dst.base.is_wide());
		// 	return;
		// }

		if (dst.is_simple() && src.is_memreg() && dst.base.size != BYTE) {
			put_inst_std(0b10101111, src, dst.base.reg, true);
			return;
		}

		if (dst.is_simple() && src.is_immediate()) {
			put_imul(dst, dst, src);
			return;
		}

		throw std::runtime_error{"Invalid operands!"};
	}

	/// Integer multiply (Signed), triple arg version
	void BufferWriter::put_imul(Location dst, Location src, Location val) {
		if (dst.is_simple() && src.is_memreg() && val.is_immediate() && dst.base.size != BYTE) {
			put_inst_std(0b011010, src, dst.base.reg, true /* TODO: sign flag */, true);

			// not sure why but it looks like IMUL uses 8bit immediate values
			put_byte(val.offset);
			return;
		}

		throw std::runtime_error{"Invalid operands!"};
	}

	/// Divide (Unsigned)
	void BufferWriter::put_div(Location src) {
		if (src.is_memreg()) {
			put_inst_std(0b111101, src, 0b110, true, src.size != BYTE);
			return;
		}

		throw std::runtime_error{"Invalid operand!"};
	}

	/// Integer divide (Signed)
	void BufferWriter::put_idiv(Location src) {
		if (src.is_memreg()) {
			put_inst_std(0b111101, src, 0b111, true, src.size != BYTE);
			return;
		}

		throw std::runtime_error{"Invalid operand!"};
	}

	/// Invert
	void BufferWriter::put_not(Location dst) {
		if (dst.is_memreg()) {
			put_inst_std(0b111101, dst, 0b010, true, dst.base.is_wide());
			return;
		}

		throw std::runtime_error{"Invalid operand!"};
	}

	/// Rotate Left
	void BufferWriter::put_rol(Location dst, Location src) {

		//       + - + ------- + - +
		// CF <- | M |   ...   | L | <- MB
		//       + - + ------- + - +
		//       |             |
		//       |             \_ Least Significant Bit (LB)
		//       \_ Most Significant Bit (MB)

		put_inst_shift(dst, src, INST_ROL);
	}

	/// Rotate Right
	void BufferWriter::put_ror(Location dst, Location src) {

		//       + - + ------- + - +
		// LB -> | M |   ...   | L | -> CF
		//       + - + ------- + - +
		//       |             |
		//       |             \_ Least Significant Bit (LB)
		//       \_ Most Significant Bit (MB)

		put_inst_shift(dst, src, INST_ROR);
	}

	/// Rotate Left Through Carry
	void BufferWriter::put_rcl(Location dst, Location src) {

		//       + - + ------- + - +
		// CF <- | M |   ...   | L | <- CF
		//       + - + ------- + - +
		//       |             |
		//       |             \_ Least Significant Bit (LB)
		//       \_ Most Significant Bit (MB)

		put_inst_shift(dst, src, INST_RCL);
	}

	/// Rotate Right Through Carry
	void BufferWriter::put_rcr(Location dst, Location src) {

		//       + - + ------- + - +
		// CF -> | M |   ...   | L | -> CF
		//       + - + ------- + - +
		//       |             |
		//       |             \_ Least Significant Bit (LB)
		//       \_ Most Significant Bit (MB)

		put_inst_shift(dst, src, INST_RCR);
	}

	/// Shift Left
	void BufferWriter::put_shl(Location dst, Location src) {

		//       + - + ------- + - +
		// CF <- | M |   ...   | L | <- 0
		//       + - + ------- + - +
		//       |             |
		//       |             \_ Least Significant Bit (LB)
		//       \_ Most Significant Bit (MB)

		put_inst_shift(dst, src, INST_SHL);
	}

	/// Shift Right
	void BufferWriter::put_shr(Location dst, Location src) {

		//       + - + ------- + - +
		//  0 -> | M |   ...   | L | -> CF
		//       + - + ------- + - +
		//       |             |
		//       |             \_ Least Significant Bit (LB)
		//       \_ Most Significant Bit (MB)

		put_inst_shift(dst, src, INST_SHR);
	}

	/// Arithmetic Shift Left
	void BufferWriter::put_sal(Location dst, Location src) {

		//       + - + ------- + - +
		// CF <- | M |   ...   | L | <- 0
		//       + - + ------- + - +
		//       |             |
		//       |             \_ Least Significant Bit (LB)
		//       \_ Most Significant Bit (MB)

		// Works the exact same way SHL does
		put_inst_shift(dst, src, INST_SHL);
	}

	/// Arithmetic Shift Right
	void BufferWriter::put_sar(Location dst, Location src) {

		//       + - + ------- + - +
		// MB -> | M |   ...   | L | -> CF
		//       + - + ------- + - +
		//       |             |
		//       |             \_ Least Significant Bit (LB)
		//       \_ Most Significant Bit (MB)

		put_inst_shift(dst, src, INST_SAR);
	}

	/// Unconditional Jump
	void BufferWriter::put_jmp(Location dst) {

		if (dst.is_labeled()) {

			// TODO shift
			Label label = dst.label;

			if (has_label(label)) {
				int offset = get_label(label) - buffer.size();

				if (offset > -127) {
					put_byte(0b11101011);
					put_label(label, BYTE);
				}

			}

			put_byte(0b11101001);
			put_label(label, DWORD);
			return;
		}

		if (dst.is_memreg()) {
			put_inst_std(0b11111111, dst, 0b100);
			return;
		}

		throw std::runtime_error{"Invalid operand!"};
	}

	/// Procedure Call
	void BufferWriter::put_call(Location dst) {

		if (dst.is_labeled()) {

			// TODO shift
			put_byte(0b11101000);
			put_label(dst.label, DWORD);
			return;

		}

		if (dst.is_memreg()) {
			put_inst_std(0b11111111, dst, 0b010);
			return;
		}

		throw std::runtime_error{"Invalid operand!"};
	}

	/// Jump on Overflow
	Label BufferWriter::put_jo(Label label) {
		put_inst_jx(label, 0b01110000, 0b10000000);
		return label;
	}

	/// Jump on Not Overflow
	Label BufferWriter::put_jno(Label label) {
		put_inst_jx(label, 0b01110001, 0b10000001);
		return label;
	}

	/// Jump on Below
	Label BufferWriter::put_jb(Label label) {
		put_inst_jx(label, 0b01110010, 0b10000010);
		return label;
	}

	/// Jump on Not Below
	Label BufferWriter::put_jnb(Label label) {
		put_inst_jx(label, 0b01110011, 0b10000011);
		return label;
	}

	/// Jump on Equal
	Label BufferWriter::put_je(Label label) {
		put_inst_jx(label, 0b01110100, 0b10000100);
		return label;
	}

	/// Jump on Not Equal
	Label BufferWriter::put_jne(Label label) {
		put_inst_jx(label, 0b01110101, 0b10000101);
		return label;
	}

	/// Jump on Below or Equal
	Label BufferWriter::put_jbe(Label label) {
		put_inst_jx(label, 0b01110110, 0b10000110);
		return label;
	}

	/// Jump on Not Below or Equal
	Label BufferWriter::put_jnbe(Label label) {
		put_inst_jx(label, 0b01110111, 0b10000111);
		return label;
	}

	/// Jump on Sign
	Label BufferWriter::put_js(Label label) {
		put_inst_jx(label, 0b01111000, 0b10001000);
		return label;
	}

	/// Jump on Not Sign
	Label BufferWriter::put_jns(Label label) {
		put_inst_jx(label, 0b01111001, 0b10001001);
		return label;
	}

	/// Jump on Parity
	Label BufferWriter::put_jp(Label label) {
		put_inst_jx(label, 0b01111010, 0b10001010);
		return label;
	}

	/// Jump on Not Parity
	Label BufferWriter::put_jnp(Label label) {
		put_inst_jx(label, 0b01111011, 0b10001011);
		return label;
	}

	/// Jump on Less
	Label BufferWriter::put_jl(Label label) {
		put_inst_jx(label, 0b01111100, 0b10001100);
		return label;
	}

	/// Jump on Not Less
	Label BufferWriter::put_jnl(Label label) {
		put_inst_jx(label, 0b01111101, 0b10001101);
		return label;
	}

	/// Jump on Less or Equal
	Label BufferWriter::put_jle(Label label) {
		put_inst_jx(label, 0b01111110, 0b10001110);
		return label;
	}

	/// Jump on Not Less or Equal
	Label BufferWriter::put_jnle(Label label) {
		put_inst_jx(label, 0b01111111, 0b10001111);
		return label;
	}

	/// No Operation
	void BufferWriter::put_nop() {
		put_byte(0b10010000);
	}

	/// Halt
	void BufferWriter::put_hlt() {
		put_byte(0b11110100);
	}

	/// Wait
	void BufferWriter::put_wait() {
		put_byte(0b10011011);
	}

	/// Push All
	void BufferWriter::put_pusha() {
		put_byte(0b01100000);
	}

	/// Pop All
	void BufferWriter::put_popa() {
		put_byte(0b01100001);
	}

	/// Push Flags
	void BufferWriter::put_pushf() {
		put_byte(0b10011100);
	}

	/// Pop Flags
	void BufferWriter::put_popf() {
		put_byte(0b10011101);
	}

	/// Clear Carry
	void BufferWriter::put_clc() {
		put_byte(0b11111000);
	}

	/// Set Carry
	void BufferWriter::put_stc() {
		put_byte(0b11111001);
	}

	/// Complement Carry Flag
	void BufferWriter::put_cmc() {
		put_byte(0b11111000);
	}

	/// Store AH into flags
	void BufferWriter::put_sahf() {
		put_byte(0b10011110);
	}

	/// Load status flags into AH register
	void BufferWriter::put_lahf() {
		put_byte(0b10011111);
	}

	/// ASCII adjust for add
	void BufferWriter::put_aaa() {
		put_byte(0b00110111);
	}

	/// Decimal adjust for add
	void BufferWriter::put_daa() {
		put_byte(0b00111111);
	}

	/// ASCII adjust for subtract
	void BufferWriter::put_aas() {
		put_byte(0b00100111);
	}

	/// Decimal adjust for subtract
	void BufferWriter::put_das() {
		put_byte(0b00101111);
	}

	/// Alias to JB, Jump on Carry
	Label BufferWriter::put_jc(Label label) {
		return put_jb(label);
	}

	/// Alias to JNB, Jump on not Carry
	Label BufferWriter::put_jnc(Label label) {
		return put_jnb(label);
	}

	/// Alias to JB, Jump on Not Above or Equal
	Label BufferWriter::put_jnae(Label label) {
		return put_jb(label);
	}

	/// Alias to JNB, Jump on Above or Equal
	Label BufferWriter::put_jae(Label label) {
		return put_jnb(label);
	}

	/// Alias to JE, Jump on Zero
	Label BufferWriter::put_jz(Label label) {
		return put_je(label);
	}

	/// Alias to JNE, Jump on Not Zero
	Label BufferWriter::put_jnz(Label label) {
		return put_jne(label);
	}

	/// Alias to JBE, Jump on Not Above
	Label BufferWriter::put_jna(Label label) {
		return put_jbe(label);
	}

	/// Alias to JNBE, Jump on Above
	Label BufferWriter::put_ja(Label label) {
		return put_jnbe(label);
	}

	/// Alias to JP, Jump on Parity Even
	Label BufferWriter::put_jpe(Label label) {
		return put_jp(label);
	}

	/// Alias to JNP, Jump on Parity Odd
	Label BufferWriter::put_jpo(Label label) {
		return put_jnp(label);
	}

	/// Alias to JL, Jump on Not Greater or Equal
	Label BufferWriter::put_jnge(Label label) {
		return put_jl(label);
	}

	/// Alias to JNL, Jump on Greater or Equal
	Label BufferWriter::put_jge(Label label) {
		return put_jnl(label);
	}

	/// Alias to JLE, Jump on Not Greater
	Label BufferWriter::put_jng(Label label) {
		return put_jle(label);
	}

	/// Alias to JNLE, Jump on Greater
	Label BufferWriter::put_jg(Label label) {
		return put_jnle(label);
	}

	/// Jump on CX Zero
	Label BufferWriter::put_jcxz(Label label) {
		put_inst_16bit_address_mark();
		return put_jecxz(label);
	}

	/// Jump on ECX Zero
	Label BufferWriter::put_jecxz(Label label) {
		put_byte(0b11100011);
		put_label(label, BYTE);
		return label;
	}

	/// Loop Times
	Label BufferWriter::put_loop(Label label) {
		put_byte(0b11100010);
		put_label(label, BYTE);
		return label;
	}

	/// ASCII adjust for division
	void BufferWriter::put_aad() {

		// the operation performed by AAD:
		// AH = AL + AH * 10
		// AL = 0

		put_word(0b00001010'11010101);
	}

	/// ASCII adjust for multiplication
	void BufferWriter::put_aam() {

		// the operation performed by AAM:
		// AH = AL div 10
		// AL = AL mod 10

		put_word(0b00001010'11010100);
	}

	/// Convert byte to word
	void BufferWriter::put_cbw() {
		put_byte(0b10011000);
	}

	/// Convert word to double word
	void BufferWriter::put_cwd() {
		put_byte(0b10011001);
	}

	/// Return from procedure
	void BufferWriter::put_ret(uint16_t bytes) {
		if (bytes != 0) {
			put_byte(0b11000010);
			put_word(bytes);
			return;
		}

		put_byte(0b11000011);
	}

}