#include "asm/x86/writer.hpp"

namespace asmio::x86 {

	/// Repeat
	BufferWriter& BufferWriter::put_rep() {
		return put_repnz();
	}

	/// Repeat while equal
	BufferWriter& BufferWriter::put_repe() {
		return put_repz();
	}

	/// Repeat while zero
	BufferWriter& BufferWriter::put_repz() {
		put_byte(0b11110011);
		return *this;
	}

	/// Repeat while not equal
	BufferWriter& BufferWriter::put_repne() {
		return put_repnz();
	}

	/// Repeat while not zero
	BufferWriter& BufferWriter::put_repnz() {
		put_byte(0b11110010);
		return *this;
	}

	/// Move byte from [ESI] to [EDI]
	void BufferWriter::put_movsb() {
		put_byte(INST_MOVS);
	}

	/// Move word from [ESI] to [EDI]
	void BufferWriter::put_movsw() {
		put_16bit_operand_prefix();
		put_byte(INST_MOVS | 1);
	}

	/// Move dword from [ESI] to [EDI]
	void BufferWriter::put_movsd() {
		put_byte(INST_MOVS | 1);
	}

	/// Input byte from I/O port specified in DX into [EDI]
	void BufferWriter::put_insb() {
		put_byte(INST_INS);
	}

	/// Input word from I/O port specified in DX into [EDI]
	void BufferWriter::put_insw() {
		put_16bit_operand_prefix();
		put_byte(INST_INS | 1);
	}

	/// Input dword from I/O port specified in DX into [EDI]
	void BufferWriter::put_insd() {
		put_byte(INST_INS | 1);
	}

	/// Output byte from [ESI] to I/O port specified in DX
	void BufferWriter::put_outsb() {
		put_byte(INST_OUTS);
	}

	/// Output word from [ESI] to I/O port specified in DX
	void BufferWriter::put_outsw() {
		put_16bit_operand_prefix();
		put_byte(INST_OUTS | 1);
	}

	/// Output dword from [ESI] to I/O port specified in DX
	void BufferWriter::put_outsd() {
		put_byte(INST_OUTS | 1);
	}

	/// Compares byte at [ESI] with byte at [EDI]
	void BufferWriter::put_cmpsb() {
		put_byte(INST_CMPS);
	}

	/// Compares word at [ESI] with word at [EDI]
	void BufferWriter::put_cmpsw() {
		put_16bit_operand_prefix();
		put_byte(INST_CMPS | 1);
	}

	/// Compares dword at [ESI] with dword at [EDI]
	void BufferWriter::put_cmpsd() {
		put_byte(INST_CMPS | 1);
	}

	/// Compare AL with byte at [EDI]
	void BufferWriter::put_scasb() {
		put_byte(INST_SCAS);
	}

	/// Compare AX with word at [EDI]
	void BufferWriter::put_scasw() {
		put_16bit_operand_prefix();
		put_byte(INST_SCAS | 1);
	}

	/// Compare EAX with dword at [EDI]
	void BufferWriter::put_scasd() {
		put_byte(INST_SCAS | 1);
	}

	/// Load byte at [ESI] into AL
	void BufferWriter::put_lodsb() {
		put_byte(INST_LODS);
	}

	/// Load word at [ESI] into AX
	void BufferWriter::put_lodsw() {
		put_16bit_operand_prefix();
		put_byte(INST_LODS | 1);
	}

	/// Load dword at [ESI] into EAX
	void BufferWriter::put_lodsd() {
		put_byte(INST_LODS | 1);
	}

	/// Store byte AL at address [EDI]
	void BufferWriter::put_stosb() {
		put_byte(INST_STOS);
	}

	/// Store word AX at address [EDI]
	void BufferWriter::put_stosw() {
		put_16bit_operand_prefix();
		put_byte(INST_STOS | 1);
	}

	/// Store dword EAX at address [EDI]
	void BufferWriter::put_stosd() {
		put_byte(INST_STOS | 1);
	}

	/// Move
	void BufferWriter::put_mov(Location dst, Location src) {

		// short form, VAL to REG
		if (src.is_immediate() && dst.is_simple()) {
			if (dst.size == WORD) {
				put_16bit_operand_prefix();
			}

			if (dst.base.is(Registry::REX)) {
				put_byte(pack_rex(dst.size == QWORD, false, false, dst.base.reg & 0b1000));
			}

			put_byte((0b1011 << 4) | (dst.is_wide() << 3) | dst.base.low());
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

		throw std::runtime_error {"Invalid operands"};
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
		if (dst.base.size < DWORD) {
			throw std::runtime_error {"Invalid operands, non-dword destination register can't be used here"};
		}

		// handle EXP to REG
		if (dst.is_simple() && !src.reference) {
			put_inst_std(0b10001101, src, dst.base.pack(), DWORD);
			return;
		}

		if (src.reference) {
			throw std::runtime_error {"Invalid operands, reference can't be used here"};
		}

		throw std::runtime_error {"Invalid operands"};
	}

	/// Exchange
	void BufferWriter::put_xchg(Location dst, Location src) {

		const uint8_t opr_size = pair_size(src, dst);

		if (dst.is_simple() && src.is_memreg()) {
			put_inst_std_ds(0b100001, src, dst.base.pack(), opr_size, true);
			return;
		}

		if (dst.is_memreg() && src.is_simple()) {
			put_inst_std_ds(0b100001, dst, src.base.pack(), opr_size, true);
			return;
		}

		throw std::runtime_error {"Invalid operands"};
	}

	/// Push
	void BufferWriter::put_push(Location src) {

		// TODO allow indeterminate pushes
		if (src.is_indeterminate()) {
			throw std::runtime_error {"Operand can't be of indeterminate size"};
		}

		// handle immediate data
		if (src.is_immediate()) {
			uint8_t imm_len = util::min_bytes(src.offset);

			if (imm_len == BYTE) {
				put_byte(0b01101010);
				put_inst_label_imm(src, BYTE);
			} else {

				// we would lose information otherwise
				if (imm_len > DWORD) {
					throw std::runtime_error {"Invalid operand, immediate value exceeds bounds"};
				}

				put_byte(0b01101000);
				put_inst_label_imm(src, DWORD);
			}

			return;
		}

		// for some reason push & pop don't handle the wide flag,
		// so we can only accept wide registers
		if (src.size != WORD && src.size != QWORD) {
			throw std::runtime_error {"Invalid operand, byte/dword can't be used here"};
		}

		// short-form
		if (src.is_simple()) {

			if (src.base.size == WORD) {
				put_16bit_operand_prefix();
			}

			put_byte((0b01010 << 3) | src.base.reg);
			return;
		}

		if (src.is_memory()) {
			put_inst_std_as(0b11111111, src, RegInfo::raw(0b110));
			return;
		}

		throw std::runtime_error {"Invalid operand"};
	}

	/// Pop
	void BufferWriter::put_pop(Location src) {

		// for some reason push & pop don't handle the wide flag,
		// so we can only accept wide registers
		if (!src.is_wide()) {
			throw std::runtime_error {"Invalid operands, byte register can't be used here"};
		}

		// short-form
		if (src.is_simple()) {

			if (src.base.size == WORD) {
				put_16bit_operand_prefix();
			}

			put_byte((0b01011 << 3) | src.base.reg);
			return;
		}

		if (src.is_memreg()) {
			put_inst_std_as(0b10001111, src, RegInfo::raw(0b000));
			return;
		}

		throw std::runtime_error {"Invalid operand"};
	}

	void BufferWriter::put_pop() {
		put_add(RSP, QWORD);
	}


	/// Increment
	void BufferWriter::put_inc(Location dst) {

		// short-form
		// if (dst.is_simple() && dst.is_wide()) {
		// 	Registry dst_reg = dst.base;
		//
		// 	if (dst.base.size == WORD) {
		// 		put_inst_16bit_operand_mark();
		// 	}
		//
		// 	put_byte((0b01000 << 3) | dst_reg.reg);
		// 	return;
		// }

		if (dst.is_indeterminate()) {
			throw std::runtime_error {"Operand can't be of indeterminate size"};
		}

		if (dst.is_memreg()) {
			put_inst_std_ds(0b111111, dst, RegInfo::raw(0b000), dst.size, true);
			return;
		}

		throw std::runtime_error {"Invalid operand"};
	}

	/// Decrement
	void BufferWriter::put_dec(Location dst) {

		// short-form
		// if (dst.is_simple() && dst.is_wide()) {
		//
		// 	if (dst.base.size == WORD) {
		// 		put_inst_16bit_operand_mark();
		// 	}
		//
		// 	put_byte((0b01001 << 3) | dst.base.reg);
		// 	return;
		// }

		if (dst.is_indeterminate()) {
			throw std::runtime_error {"Operand can't be of indeterminate size"};
		}

		if (dst.is_memreg()) {
			put_inst_std_ds(0b111111, dst, RegInfo::raw(0b001), dst.size, true);
			return;
		}

		throw std::runtime_error {"Invalid operand"};
	}

	/// Negate
	void BufferWriter::put_neg(Location dst) {
		if (dst.is_memreg()) {
			put_inst_std_ds(0b111101, dst, RegInfo::raw(0b011), dst.size, true);
			return;
		}

		throw std::runtime_error {"Invalid operand"};
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

	/// Bit Scan Forward
	void BufferWriter::put_bsf(Location dst, Location src) {
		uint32_t size = pair_size(src, dst);

		// FIXME
		if (size != WORD && size != DWORD) {
			throw std::runtime_error {"Invalid operand size, expected word or dword"};
		}

		if (dst.is_simple() && src.is_memreg()) {
			put_inst_std(0b10111100, src, dst.base.pack(), size, true);
			return;
		}

		throw std::runtime_error {"Invalid operands"};
	}

	/// Bit Scan Reverse
	void BufferWriter::put_bsr(Location dst, Location src) {
		uint32_t size = pair_size(src, dst);

		// FIXME
		if (size != WORD && size != DWORD) {
			throw std::runtime_error {"Invalid operand size, expected word or dword"};
		}

		if (dst.is_simple() && src.is_memreg()) {
			put_inst_std(0b10111101, src, dst.base.pack(), size, true);
			return;
		}

		throw std::runtime_error {"Invalid operands"};
	}

	/// Multiply (Unsigned)
	void BufferWriter::put_mul(Location src) {

		if (src.is_indeterminate()) {
			throw std::runtime_error {"Operand can't be of indeterminate size"};
		}

		if (src.is_memreg()) {
			put_inst_std_ds(0b111101, src, RegInfo::raw(0b100), src.size, true);
			return;
		}

		throw std::runtime_error {"Invalid operand"};
	}

	/// Integer multiply (Signed)
	void BufferWriter::put_imul(Location dst, Location src) {

		// short form
		if (dst.is_simple() && src.is_memreg() && src.size == dst.size && dst.base.is(Registry::ACCUMULATOR)) {
			put_inst_std_ds(0b111101, src, RegInfo::raw(0b101), pair_size(src, dst), true);
		 	return;
		}

		if (dst.is_simple() && src.is_memreg() && dst.base.size != BYTE) {
			put_inst_std(0b10101111, src, dst.base.pack(), pair_size(src, dst), true);
			return;
		}

		if (dst.is_simple() && src.is_immediate()) {
			put_imul(dst, dst, src);
			return;
		}

		throw std::runtime_error {"Invalid operands"};
	}

	/// Integer multiply (Signed), triple arg version
	void BufferWriter::put_imul(Location dst, Location src, Location val) {

		if (dst.base.size == BYTE) {
			throw std::runtime_error {"Invalid operand, byte register can't be used here"};
		}

		if (dst.is_simple() && src.is_memreg() && val.is_immediate()) {
			put_inst_std_dw(0b011010, src, dst.base.pack(), pair_size(src, dst), true /* TODO: sign flag */, true);

			// not sure why but it looks like IMUL uses 8bit immediate values
			put_byte(val.offset);
			return;
		}

		throw std::runtime_error {"Invalid operands"};
	}

	/// Divide (Unsigned)
	void BufferWriter::put_div(Location src) {
		if (src.is_memreg()) {
			put_inst_std_ds(0b111101, src, RegInfo::raw(0b110), src.size, true);
			return;
		}

		throw std::runtime_error {"Invalid operand"};
	}

	/// Integer divide (Signed)
	void BufferWriter::put_idiv(Location src) {
		if (src.is_memreg()) {
			put_inst_std_ds(0b111101, src, RegInfo::raw(0b111), src.size, true);
			return;
		}

		throw std::runtime_error {"Invalid operand"};
	}

	/// Invert
	void BufferWriter::put_not(Location dst) {
		if (dst.is_memreg()) {
			put_inst_std_ds(0b111101, dst, RegInfo::raw(0b010), dst.size, true);
			return;
		}

		throw std::runtime_error {"Invalid operand"};
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

	/// Double Left Shift
	void BufferWriter::put_shld(Location dst, Location src, Location cnt) {

		//       + - + ------- + - +    + - + ------- + - +
		// CF <- | M |   dst   | L | <- | M |   src   | L |
		//       + - + ------- + - +    + - + ------- + - +
		//       |             |
		//       |             \_ Least Significant Bit (LB)
		//       \_ Most Significant Bit (MB)

		put_inst_double_shift(0b1010'0100, dst, src, cnt);
	}

	/// Double Right Shift
	INST BufferWriter::put_shrd(Location dst, Location src, Location cnt) {

		//       + - + ------- + - +    + - + ------- + - +
		//       | M |   src   | L | -> | M |   dst   | L | -> CF
		//       + - + ------- + - +    + - + ------- + - +
		//       |             |
		//       |             \_ Least Significant Bit (LB)
		//       \_ Most Significant Bit (MB)

		put_inst_double_shift(0b1010'1100, dst, src, cnt);
	}

	/// Unconditional Jump
	void BufferWriter::put_jmp(Location dst) {

		if (dst.is_jump_label()) {
			Label label = dst.label;
			long shift = dst.offset;

			if (has_label(label)) {
				int offset = get_label(label) - buffer.size() + shift;

				if (offset > -127) {
					put_byte(0b11101011);
					put_label(label, BYTE, shift);
					return;
				}
			}

			put_byte(0b11101001);
			put_label(label, DWORD, shift);
			return;
		}

		if (dst.is_memreg()) {
			put_inst_std_as(0b11111111, dst, RegInfo::raw(0b100));
			return;
		}

		throw std::runtime_error {"Invalid operand"};
	}

	/// Procedure Call
	void BufferWriter::put_call(Location dst) {

		if (dst.is_jump_label()) {
			put_byte(0b11101000);
			put_label(dst.label, DWORD, dst.offset);
			return;
		}

		if (dst.is_memreg()) {
			put_inst_std_as(0b11111111, dst, RegInfo::raw(0b010));
			return;
		}

		throw std::runtime_error {"Invalid operand"};
	}

	/// Jump on Overflow
	void BufferWriter::put_jo(Location label) {
		put_inst_jx(label, 0b01110000, 0b10000000);
	}

	/// Jump on Not Overflow
	void BufferWriter::put_jno(Location label) {
		put_inst_jx(label, 0b01110001, 0b10000001);
	}

	/// Jump on Below
	void BufferWriter::put_jb(Location label) {
		put_inst_jx(label, 0b01110010, 0b10000010);
	}

	/// Jump on Not Below
	void BufferWriter::put_jnb(Location label) {
		put_inst_jx(label, 0b01110011, 0b10000011);
	}

	/// Jump on Equal
	void BufferWriter::put_je(Location label) {
		put_inst_jx(label, 0b01110100, 0b10000100);
	}

	/// Jump on Not Equal
	void BufferWriter::put_jne(Location label) {
		put_inst_jx(label, 0b01110101, 0b10000101);
	}

	/// Jump on Below or Equal
	void BufferWriter::put_jbe(Location label) {
		put_inst_jx(label, 0b01110110, 0b10000110);
	}

	/// Jump on Not Below or Equal
	void BufferWriter::put_jnbe(Location label) {
		put_inst_jx(label, 0b01110111, 0b10000111);
	}

	/// Jump on Sign
	void BufferWriter::put_js(Location label) {
		put_inst_jx(label, 0b01111000, 0b10001000);
	}

	/// Jump on Not Sign
	void BufferWriter::put_jns(Location label) {
		put_inst_jx(label, 0b01111001, 0b10001001);
	}

	/// Jump on Parity
	void BufferWriter::put_jp(Location label) {
		put_inst_jx(label, 0b01111010, 0b10001010);
	}

	/// Jump on Not Parity
	void BufferWriter::put_jnp(Location label) {
		put_inst_jx(label, 0b01111011, 0b10001011);
	}

	/// Jump on Less
	void BufferWriter::put_jl(Location label) {
		put_inst_jx(label, 0b01111100, 0b10001100);
	}

	/// Jump on Not Less
	void BufferWriter::put_jnl(Location label) {
		put_inst_jx(label, 0b01111101, 0b10001101);
	}

	/// Jump on Less or Equal
	void BufferWriter::put_jle(Location label) {
		put_inst_jx(label, 0b01111110, 0b10001110);
	}

	/// Jump on Not Less or Equal
	void BufferWriter::put_jnle(Location label) {
		put_inst_jx(label, 0b01111111, 0b10001111);
	}

	/// Set Byte on Overflow
	void BufferWriter::put_seto(Location dst) {
		put_inst_setx(dst, 0);
	}

	/// Set Byte on Not Overflow
	void BufferWriter::put_setno(Location dst) {
		put_inst_setx(dst, 1);
	}

	/// Set Byte on Below
	void BufferWriter::put_setb(Location dst) {
		put_inst_setx(dst, 2);
	}

	/// Set Byte on Not Below
	void BufferWriter::put_setnb(Location dst) {
		put_inst_setx(dst, 3);
	}

	/// Set Byte on Equal
	void BufferWriter::put_sete(Location dst) {
		put_inst_setx(dst, 4);
	}

	/// Set Byte on Not Equal
	void BufferWriter::put_setne(Location dst) {
		put_inst_setx(dst, 5);
	}

	/// Set Byte on Below or Equal
	void BufferWriter::put_setbe(Location dst) {
		put_inst_setx(dst, 6);
	}

	/// Set Byte on Not Below or Equal
	void BufferWriter::put_setnbe(Location dst) {
		put_inst_setx(dst, 7);
	}

	/// Set Byte on Sign
	void BufferWriter::put_sets(Location dst) {
		put_inst_setx(dst, 8);
	}

	/// Set Byte on Not Sign
	void BufferWriter::put_setns(Location dst) {
		put_inst_setx(dst, 9);
	}

	/// Set Byte on Parity
	void BufferWriter::put_setp(Location dst) {
		put_inst_setx(dst, 10);
	}

	/// Set Byte on Not Parity
	void BufferWriter::put_setnp(Location dst) {
		put_inst_setx(dst, 11);
	}

	/// Set Byte on Less
	void BufferWriter::put_setl(Location dst) {
		put_inst_setx(dst, 12);
	}

	/// Set Byte on Not Less
	void BufferWriter::put_setnl(Location dst) {
		put_inst_setx(dst, 13);
	}

	/// Set Byte on Less or Equal
	void BufferWriter::put_setle(Location dst) {
		put_inst_setx(dst, 14);
	}

	/// Set Byte on Not Less or Equal
	void BufferWriter::put_setnle(Location dst) {
		put_inst_setx(dst, 15);
	}

	/// Interrupt
	void BufferWriter::put_int(Location type) {

		if (!type.is_immediate()) {
			throw std::runtime_error {"Invalid operand"};
		}

		// short form
		if (type.offset == 3) {
			put_byte(0xCC);
			return;
		}

		// standard form
		put_byte(0b11001101);
		put_byte(type.offset);

	}

	/// Interrupt if Overflow
	void BufferWriter::put_into() {
		put_byte(0b11001110);
	}

	/// Return from Interrupt
	void BufferWriter::put_iret() {
		put_byte(0b11001111);
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

	/// Undefined Instruction
	void BufferWriter::put_ud2() {
		put_byte(0x0F);
		put_byte(0x0B);
	}

	/// Enter Procedure
	void BufferWriter::put_enter(Location alc, Location nst) {
		if (alc.is_immediate() && nst.is_immediate()) {
			put_byte(0b11001000);
			put_word(alc.offset);
			put_byte(nst.offset);
			return;
		}

		throw std::runtime_error {"Invalid operands"};
	}

	/// Leave Procedure
	void BufferWriter::put_leave() {
		put_byte(0b11001001);
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
	void BufferWriter::put_pushfd() {
		put_byte(0b10011100);
	}

	/// Pop Flags
	void BufferWriter::put_popfd() {
		put_byte(0b10011101);
	}

	/// Push Flags
	void BufferWriter::put_pushf() {
		put_16bit_operand_prefix();
		put_pushfd();
	}

	/// Pop Flags
	void BufferWriter::put_popf() {
		put_16bit_operand_prefix();
		put_popfd();
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
		put_byte(0b11110101);
	}

	/// Clear Direction Flag
	void BufferWriter::put_cld() {
		put_byte(0b11111100);
	}

	/// Set Direction Flag
	void BufferWriter::put_std() {
		put_byte(0b11111101);
	}

	/// Clear Interrupt Flag
	void BufferWriter::put_cli() {
		put_byte(0b11111010);
	}

	/// Set Interrupt Flag
	void BufferWriter::put_sti() {
		put_byte(0b11111011);
	}

	/// Set Interrupt Flag to Immediate, ASMIOV extension
	void BufferWriter::put_sif(Location src) {
		if (!src.is_immediate()) {
			throw std::runtime_error {"Invalid operand"};
		}

		if (src.offset == 0) put_cli(); else put_sti();
	}

	/// Set Carry Flag to Immediate, ASMIOV extension
	void BufferWriter::put_scf(Location src) {
		if (!src.is_immediate()) {
			throw std::runtime_error {"Invalid operand"};
		}

		if (src.offset == 0) put_clc(); else put_stc();
	}

	/// Set Direction Flag to Immediate, ASMIOV extension
	void BufferWriter::put_sdf(Location src) {
		if (!src.is_immediate()) {
			throw std::runtime_error {"Invalid operand"};
		}

		if (src.offset == 0) put_cld(); else put_std();
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
	void BufferWriter::put_jc(Location label) {
		put_jb(label);
	}

	/// Alias to JNB, Jump on not Carry
	void BufferWriter::put_jnc(Location label) {
		put_jnb(label);
	}

	/// Alias to JB, Jump on Not Above or Equal
	void BufferWriter::put_jnae(Location label) {
		put_jb(label);
	}

	/// Alias to JNB, Jump on Above or Equal
	void BufferWriter::put_jae(Location label) {
		put_jnb(label);
	}

	/// Alias to JE, Jump on Zero
	void BufferWriter::put_jz(Location label) {
		put_je(label);
	}

	/// Alias to JNE, Jump on Not Zero
	void BufferWriter::put_jnz(Location label) {
		put_jne(label);
	}

	/// Alias to JBE, Jump on Not Above
	void BufferWriter::put_jna(Location label) {
		put_jbe(label);
	}

	/// Alias to JNBE, Jump on Above
	void BufferWriter::put_ja(Location label) {
		put_jnbe(label);
	}

	/// Alias to JP, Jump on Parity Even
	void BufferWriter::put_jpe(Location label) {
		put_jp(label);
	}

	/// Alias to JNP, Jump on Parity Odd
	void BufferWriter::put_jpo(Location label) {
		put_jnp(label);
	}

	/// Alias to JL, Jump on Not Greater or Equal
	void BufferWriter::put_jnge(Location label) {
		put_jl(label);
	}

	/// Alias to JNL, Jump on Greater or Equal
	void BufferWriter::put_jge(Location label) {
		put_jnl(label);
	}

	/// Alias to JLE, Jump on Not Greater
	void BufferWriter::put_jng(Location label) {
		put_jle(label);
	}

	/// Alias to JNLE, Jump on Greater
	void BufferWriter::put_jg(Location label) {
		put_jnle(label);
	}

	/// Jump on CX Zero
	void BufferWriter::put_jcxz(Location label) {
		put_32bit_address_prefix();
		put_jecxz(label);
	}

	/// Jump on ECX Zero
	void BufferWriter::put_jecxz(Location dst) {

		// FIXME: no range check
		if (dst.is_jump_label()) {
			put_byte(0b11100011);
			put_label(dst.label, BYTE, dst.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};
	}

	/// Loop Times
	void BufferWriter::put_loop(Location dst) {

		// FIXME: no range check
		if (dst.is_jump_label()) {
			put_byte(0b11100010);
			put_label(dst.label, BYTE, dst.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};
	}

	/// Loop if equal
	INST BufferWriter::put_loope(Location dst) {
		put_loopz(dst);
	}

	/// Loop if zero
	INST BufferWriter::put_loopz(Location dst) {

		// FIXME: no range check
		if (dst.is_jump_label()) {
			put_byte(0b11100001);
			put_label(dst.label, BYTE, dst.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};
	}

	/// Loop if not equal
	INST BufferWriter::put_loopne(Location dst) {
		put_loopnz(dst);
	}

	/// Loop if not zero
	INST BufferWriter::put_loopnz(Location dst) {

		// FIXME: no range check
		if (dst.is_jump_label()) {
			put_byte(0b11100000);
			put_label(dst.label, BYTE, dst.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};
	}

	/// Alias to SETB, Set Byte on Carry
	void BufferWriter::put_setc(Location dst) {
		put_setb(dst);
	}

	/// Alias to SETNB, Set Byte on not Carry
	void BufferWriter::put_setnc(Location dst) {
		put_setnb(dst);
	}

	/// Alias to SETB, Set Byte on Not Above or Equal
	void BufferWriter::put_setnae(Location dst) {
		put_setb(dst);
	}

	/// Alias to SETNB, Set Byte on Above or Equal
	void BufferWriter::put_setae(Location dst) {
		put_setnb(dst);
	}

	/// Alias to SETE, Set Byte on Zero
	void BufferWriter::put_setz(Location dst) {
		put_sete(dst);
	}

	/// Alias to SETNE, Set Byte on Not Zero
	void BufferWriter::put_setnz(Location dst) {
		put_setne(dst);
	}

	/// Alias to SETBE, Set Byte on Not Above
	void BufferWriter::put_setna(Location dst) {
		put_setbe(dst);
	}

	/// Alias to SETNBE, Set Byte on Above
	void BufferWriter::put_seta(Location dst) {
		put_setnbe(dst);
	}

	/// Alias to SETP, Set Byte on Parity Even
	void BufferWriter::put_setpe(Location dst) {
		put_setp(dst);
	}

	/// Alias to SETNP, Set Byte on Parity Odd
	void BufferWriter::put_setpo(Location dst) {
		put_setnp(dst);
	}

	/// Alias to SETL, Set Byte on Not Greater or Equal
	void BufferWriter::put_setnge(Location dst) {
		put_setl(dst);
	}

	/// Alias to SETNL, Set Byte on Greater or Equal
	void BufferWriter::put_setge(Location dst) {
		put_setnl(dst);
	}

	/// Alias to SETLE, Set Byte on Not Greater
	void BufferWriter::put_setng(Location dst) {
		put_setle(dst);
	}

	/// Alias to SETNLE, Set Byte on Greater
	void BufferWriter::put_setg(Location dst) {
		put_setnle(dst);
	}

	/// Convert byte to word
	void BufferWriter::put_cbw() {
		put_byte(0b10011000);
	}

	/// Convert word to double word
	void BufferWriter::put_cwd() {
		put_byte(0b10011001);
	}

	/// Table Look-up Translation
	void BufferWriter::put_xlat() {
		put_byte(0b11010111);
	}

	/// Input from Port
	void BufferWriter::put_in(Location dst, Location src) {

		if (!dst.is_simple() || !(dst.base.is(EAX) || dst.base.is(AX) || dst.base.is(AL))) {
			throw std::runtime_error {"Invalid destination operand, expected EAX, AX or AL registers"};
		}

		if (dst.size == WORD) {
			put_16bit_operand_prefix();
		}

		if (src.is_immediate()) {
			put_byte(0b11100100 | dst.is_wide());
			put_byte(dst.offset);
			return;
		}

		if (src.is_simple() && src.base.is(DX)) {
			put_byte(0b11101100 | dst.is_wide());
			return;
		}

		throw std::runtime_error {"Invalid source operand, expected an immediate value or DX register"};

	}

	/// Output to Port
	void BufferWriter::put_out(Location dst, Location src) {

		if (!src.is_simple() || !(src.base.is(EAX) || src.base.is(AX) || src.base.is(AL))) {
			throw std::runtime_error {"Invalid source operand, expected EAX, AX or AL registers"};
		}

		if (src.size == WORD) {
			put_16bit_operand_prefix();
		}

		if (dst.is_immediate()) {
			put_byte(0b11100110 | src.is_wide());
			put_byte(dst.offset);
			return;
		}

		if (dst.is_simple() && dst.base.is(DX)) {
			put_byte(0b11101110 | src.is_wide());
			return;
		}

		throw std::runtime_error {"Invalid destination operand, expected an immediate value or DX register"};

	}

	/// Test For Bit Pattern
	void BufferWriter::put_test(Location dst, Location src) {

		if (src.is_memreg() && dst.is_simple()) {
			put_inst_std_ds(0b100001, src, dst.base.pack(), pair_size(src, dst), false);
			return;
		}

		if (src.is_simple() && dst.is_memreg()) {
			put_inst_std_ds(0b100001, dst, src.base.pack(), pair_size(src, dst), false);
			return;
		}

		// short form
		if (src.is_accum() && dst.is_immediate()) {
			if (src.size == WORD) {
				put_16bit_operand_prefix();
			}

			put_byte(0b10101000 | src.is_wide());
			put_inst_imm(dst.offset, src.size);
			return;
		}

		// short form
		if (src.is_immediate() && dst.is_accum()) {
			if (dst.size == WORD) {
				put_16bit_operand_prefix();
			}

			put_byte(0b10101000 | dst.is_wide());
			put_inst_imm(src.offset, dst.size);
			return;
		}

		if (src.is_immediate() && dst.is_memreg()) {
			put_inst_std_ds(0b111101, dst, RegInfo::raw(0b000), pair_size(src, dst), true);
			put_inst_imm(src.offset, dst.size);
			return;
		}

		if (src.is_memreg() && dst.is_immediate()) {
			put_inst_std_ds(0b111101, src, RegInfo::raw(0b000), pair_size(src, dst), true);
			put_inst_imm(dst.offset, src.size);
			return;
		}

		throw std::runtime_error {"Invalid operands"};
	}

	/// Sets flags accordingly to the value of register given, ASMIOV extension
	void BufferWriter::put_test(Location src) {

		if (src.is_simple()) {
			put_test(src, src);
			return;
		}

		throw std::runtime_error {"Invalid operand, register expected"};
	}

	/// Return from procedure
	void BufferWriter::put_ret() {
		put_byte(0b11000011);
	}

	/// Return from procedure and pop X bytes
	void BufferWriter::put_ret(Location loc) {
		if (loc.is_immediate()) {
			uint32_t loc_val = loc.offset;

			if (loc_val != 0) {
				put_byte(0b11000010);
				put_word(loc_val);
				return;
			}

			put_byte(0b11000011);
			return;
		}

		throw std::runtime_error {"Invalid operand"};
	}

	void BufferWriter::put_xadd(Location dst, Location src) {

		if (dst.is_memreg() && src.is_simple()) {
			put_inst_std_ds(0xC0 >> 2, dst, src.base.pack(), pair_size(dst, src), false, true);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	void BufferWriter::put_bswap(Location dst) {

		if (!dst.is_simple()) {
			throw std::runtime_error {"Invalid operand, only register can be used used here"};
		}

		if (dst.size != DWORD && dst.size != QWORD) {
			throw std::runtime_error {"Invalid operand size, expected dword/qword"};
		}

		const uint8_t reg = dst.base.reg;

		if (dst.base.is(Registry::REX)) {
			put_byte(pack_rex(dst.size == QWORD, false, false, reg & 0b1000));
		}

		put_byte(LONG_OPCODE);
		put_byte(0xC8 | reg);

	}

	void BufferWriter::put_invd() {
		put_byte(LONG_OPCODE);
		put_byte(0x08);
	}

	void BufferWriter::put_wbinvd() {
		put_byte(LONG_OPCODE);
		put_byte(0x09);
	}

	void BufferWriter::put_cmpxchg(Location dst, Location src) {

		if (dst.is_memreg() && src.is_simple()) {
			put_inst_std_ds(0xB0 >> 2, dst, src.base.pack(), pair_size(dst, src), false, true);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Convert Doubleword to Quadword
	void BufferWriter::put_cqo() {
		put_rex_w();
		put_cwd();
	}

	/// Swap GS Base Register
	void BufferWriter::put_swapgs() {
		put_byte(0x0F);
		put_byte(0x01);
		put_byte(0xF8);
	}

	/// Read From Model Specific Register
	void BufferWriter::put_rdmsr() {
		put_byte(0x0F);
		put_byte(0x32);
	}

	/// Write to Model Specific Register
	void BufferWriter::put_wrmsr() {
		put_byte(0x0F);
		put_byte(0x30);
	}

	/// Fast System Call
	void BufferWriter::put_syscall() {
		put_byte(0x0F);
		put_byte(0x05);
	}

	/// Return From Fast System Call into Long Mode
	void BufferWriter::put_sysretl() {
		put_rex_w();
		put_byte(0x0F);
		put_byte(0x07);
	}

	/// Return From Fast System Call into Compatibility Mode
	void BufferWriter::put_sysretc() {
		put_byte(0x0F);
		put_byte(0x07);
	}

}
