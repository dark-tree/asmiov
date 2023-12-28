#pragma once

#include <bitset>
#include <iomanip>
#include <utility>
#include "address.hpp"
#include "label.hpp"
#include "buffer.hpp"

namespace asmio::x86 {

	class BufferWriter {

		private:

			std::unordered_map<Label, size_t, Label::HashFunction> labels;
			std::vector<uint8_t> buffer;
			std::vector<LabelCommand> commands;

			uint8_t pack_opcode_dw(uint8_t opcode, bool d, bool w) {

				//   7 6 5 4 3 2   1   0
				// + ----------- + - + - +
				// | opcode      | d | w |
				// + ----------- + - + - +
				// |             |   |
				// |             |   \_ wide flag
				// |             \_ direction flag
				// \_ operation code

				// Wide flag controls which set of register is encoded
				// in the subsequent bytes

				// Direction flag controls the operation direction
				// with d=1 being reg <= r/m, and d=0 reg => r/m

				// Some operations expect direction to be set to a specific value,
				// so it is sometimes considered to be a part of the opcode itself

				return (opcode << 2 | (d << 1) | w);
			}

			void put_inst_mod_reg_rm(uint8_t mod, uint8_t reg, uint8_t r_m) {

				//   7 6   5 4 3   2 1 0
				// + --- + ----- + ----- +
				// | mod | reg   | r/m   |
				// + --- + ----- + ----- +
				// |     |       |
				// |     |       \_ encodes register
				// |     \_ encodes register of instruction specific data
				// \_ offset size

				// If mod is set to MOD_SHORT (0b11) then both reg and r/m are treated
				// as simple register references, for example in "mov eax, edx"

				// If mod is set to any other value (MOD_NONE, MOD_BYTE, MOD_QUAD)
				// then the r/m is treated as a pointer, for example in "mov [eax], edx"

				// Mod also controls the length of the offset in bytes, with MOD_NONE having no offset,
				// MOD_BYTE having a single byte of offset after the instruction, and MOD_QUAD having four.

				// if mod is NOT equal MOD_SHORT, and r/m is set to RM_SIB (0b100, same value as ESP)
				// then the SIB byte is expected after this byte

				put_byte(r_m | (reg << 3) | (mod << 6));
			}

			void put_inst_sib(uint8_t ss, uint8_t index, uint8_t base) {

				//   7 6   5 4 3   2 1 0
				// + --- + ----- + ----- +
				// | ss  | index | base  |
				// + --- + ----- + ----- +
				// |     |       |
				// |     |       \_ base registry
				// |     \_ index registry
				// \_ scaling factor

				// Index register can be omitted from SIB by setting 'index' to NO_SIB_INDEX (0b100) and
				// 'ss' to NO_SIB_SCALE (0b00), this is useful for encoding the ESP registry

				put_byte(base | (index << 3) | (ss << 6));
			}

			void put_inst_imm(uint32_t immediate, uint8_t width) {
				const uint8_t* imm_ptr = (uint8_t*) &immediate;

				if (width >= BYTE)  buffer.push_back(imm_ptr[0]);
				if (width >= WORD)  buffer.push_back(imm_ptr[1]);
				if (width >= DWORD) buffer.push_back(imm_ptr[2]);
				if (width >= DWORD) buffer.push_back(imm_ptr[3]);
			}

			void put_inst_sib(Location ref) {
				uint8_t ss_flag = ref.get_ss_flag();
				uint8_t index_reg = ref.index.reg;

				if (ref.index.is(UNSET)) {
					index_reg = NO_SIB_INDEX;
					ss_flag = NO_SIB_SCALE;
				}

				put_inst_sib(ss_flag, index_reg, ref.base.reg);
			}

			void put_inst_label_imm(Location imm, uint8_t size) {
				if (imm.is_labeled()) {
					commands.emplace_back(imm.label, size, buffer.size(), imm.offset, false);
				}

				put_inst_imm(imm.offset, size);
				return;
			}

			void put_inst_std(uint8_t opcode, Location dst, uint8_t reg, bool longer = false) {
				uint8_t dst_reg = dst.base.reg;

				if (dst.size == WORD) {
					put_inst_16bit_operand_mark();
				}

				// two byte opcode, starts with 0x0F
				if (longer) {
					put_byte(0x0F);
				}

				if (dst.is_simple()) {
					put_byte(opcode);
					put_inst_mod_reg_rm(MOD_SHORT, reg, dst_reg);
					return;
				}

				// we have to use the SIB byte to target ESP
				bool sib = dst.base.is(ESP) || dst.is_indexed();
				uint8_t mod = dst.get_mod_flag();

				// special case for [EBP + (any indexed) + 0]
				if (dst.base.is(EBP)) {
					mod = MOD_BYTE;
				}

				// TODO: cleanup
				if (dst.base.is(UNSET)) {
					dst_reg = NO_BASE;
				}

				put_byte(opcode);
				put_inst_mod_reg_rm(dst_reg == NO_BASE ? MOD_NONE : mod, reg, sib ? RM_SIB : dst_reg);

				if (sib) {
					put_inst_sib(dst);
				}

				if (mod == MOD_BYTE || mod == MOD_QUAD) {
					put_inst_label_imm(dst, mod == MOD_BYTE ? BYTE : DWORD);
				}
			}

			/*
			 * FIXME: Do not use this method if the 'direction' and 'wide' flags given are set to a constant value!
			 */
			void put_inst_std(uint8_t opcode, Location dst, uint8_t reg, bool direction, bool wide, bool longer = false) {
				put_inst_std(pack_opcode_dw(opcode, direction, wide), dst, reg, longer);
			}

			void put_inst_fpu(uint8_t opcode, uint8_t base, uint8_t sti = 0) {
				put_byte(opcode);
				put_byte(base + sti);
			}

			/**
			 * Used for constructing the MOV instruction
			 */
			void put_inst_mov(Location dst, Location src, bool direction) {

				// for immediate values this will equal 0
				uint8_t src_reg = src.base.reg;
				uint8_t dst_len = dst.base.size;

				put_inst_std(src.is_immediate() ? 0b110001 : 0b100010, dst, src_reg, direction, dst.base.is_wide());

				if (src.is_immediate()) {
					put_inst_label_imm(src, dst_len);
				}
			}

			/**
			 * Used for constructing the MOVSX and MOVZX instructions
			 */
			void put_inst_movx(uint8_t opcode, Location dst, Location src) {
				uint8_t dst_len = dst.size;
				uint8_t src_len = src.size;

				// 66h movsx (w=0) -> movsx [word], [byte]
				// 66h movsx (w=1) ->
				//     movsx (w=0) -> movsx [quad], [byte]
				//     movsx (w=1) -> movsx [quad], [word]

				if (!dst.is_simple()) {
					throw std::runtime_error {"Invalid destination operand!"};
				}

				if (src_len >= dst_len) {
					throw std::runtime_error {"Invalid destination sizes!"};
				}

				put_inst_std(opcode, src, dst.base.reg, true, src_len == WORD, true);
			}

			/**
			 * Used to for constructing the shift instructions
			 */
			void put_inst_shift(Location dst, Location src, uint8_t inst) {

				bool dst_wide = dst.base.is_wide();

				if (src.is_simple() && src.base.is(CL)) {
					put_inst_std(0b110100, dst, inst, true, dst_wide);

					return;
				}

				if (src.is_immediate()) {
					uint8_t src_val = src.offset;

					if (src_val == 1) {
						put_inst_std(0b110100, dst, inst, false, dst_wide);
					} else {
						put_inst_std(0b110000, dst, inst, false, dst_wide);
						put_byte(src_val);
					}

					return;
				}

				throw std::runtime_error {"Invalid operands!"};

			}

			void put_inst_tuple(Location dst, Location src, uint8_t opcode_rmr, uint8_t opcode_reg) {

				if (dst.is_simple() && src.is_memreg()) {
					put_inst_std(opcode_rmr, src, dst.base.reg, true, dst.base.is_wide());
					return;
				}

				if (src.is_simple() && dst.reference) {
					put_inst_std(opcode_rmr, src, dst.base.reg, false, src.base.is_wide());
					return;
				}

				if (dst.is_memreg() && src.is_immediate()) {
					put_inst_std(0b100000, dst, opcode_reg, false /* TODO: sign field (???) */, dst.size != BYTE);
					put_inst_label_imm(src, dst.size);
					return;
				}

				throw std::runtime_error {"Invalid operands!"};

			}

			/**
			 * Used for constructing the Bit Test family of instructions
			 */
			void put_inst_btx(Location dst, Location src, uint8_t opcode, uint8_t inst) {

				if (dst.size == WORD || dst.size == DWORD) {
					if (dst.is_memreg() && src.is_simple()) {
						put_inst_std(opcode, dst, src.base.reg, true, true, true);
						return;
					}

					if (dst.is_memreg() && src.is_immediate()) {
						put_inst_std(0b101110, dst, inst, true, false, true);
						put_byte(src.offset);
						return;
					}
				}

				throw std::runtime_error {"Invalid operands!"};

			}

			/**
			 * Used for constructing the conditional jump family of instructions
			 */
			void put_inst_jx(Label label, uint8_t sopcode, uint8_t lopcode) {

				if (has_label(label)) {
					int offset = get_label(label) - buffer.size();

					if (offset > -127) {
						put_byte(sopcode);
						put_label(label, BYTE);
						return;
					}
				}

				put_byte(0b00001111);
				put_byte(lopcode);
				put_label(label, DWORD);

			}

			void put_inst_16bit_operand_mark() {
				put_byte(0b01100110);
			}

			void put_inst_16bit_address_mark() {
				put_byte(0b01100111);
			}

			void put_label(Label label, uint8_t size) {
				commands.emplace_back(label, size, buffer.size(), 0, true);

				while (size --> 0) {
					put_byte(0);
				}
			}

			bool has_label(Label label) {
				return labels.contains(label);
			}

			int get_label(Label label) {
				return labels.at(label);
			}

		public:

			Label label(Label label) {
				if (has_label(label)) {
					throw std::runtime_error {"Invalid label, redefinition attempted!"};
				}

				labels[label] = buffer.size();
				return label;
			}

			/// Move
			void put_mov(Location dst, Location src) {

				// verify operand size
				if (src.is_simple() && dst.is_simple()) {
					if (src.base.size != dst.base.size) {
						throw std::runtime_error {"Operands must have equal size!"};
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

				throw std::runtime_error {"Invalid operands!"};
			}

			/// Move with Sign Extension
			void put_movsx(Location dst, Location src) {
				put_inst_movx(0b101111, dst, src);
			}

			/// Move with Zero Extension
			void put_movzx(Location dst, Location src) {
				put_inst_movx(0b101101, dst, src);
			}

			/// Load Effective Address
			void put_lea(Location dst, Location src) {

				// lea deals with addresses, addresses use 32 bits,
				// so this is quite a logical limitation
				if (dst.base.size != DWORD) {
					throw std::runtime_error {"Invalid operands, non-dword target register can't be used here!"};
				}

				// handle EXP to REG
				if (dst.is_simple() && !src.reference) {
					put_inst_std(0b100011, src, dst.base.reg, false, true);
					return;
				}

				if (src.reference) {
					throw std::runtime_error {"Invalid operands, reference can't be used here!"};
				}

				throw std::runtime_error {"Invalid operands!"};
			}

			/// Exchange
			void put_xchg(Location dst, Location src) {

				if (dst.is_simple() && !src.base.is(UNSET)) {
					put_inst_std(0b100001, src, dst.base.reg, true, src.base.is_wide());
					return;
				}

				if (!dst.base.is(UNSET) && src.is_simple()) {
					put_inst_std(0b100001, dst, src.base.reg, true, dst.base.is_wide());
					return;
				}

				throw std::runtime_error {"Invalid operands!"};
			}

			/// Push
			void put_push(Location src) {

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
					throw std::runtime_error {"Invalid operands, byte register can't be used here!"};
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
					put_inst_std(0b111111, 0b110, src.base.reg, true, true);
					return;
				}

				throw std::runtime_error {"Invalid operands!"};
			}

			/// Pop
			void put_pop(Location src) {

				// for some reason push & pop don't handle the wide flag,
				// so we can only accept wide registers
				if (!src.base.is_wide()) {
					throw std::runtime_error {"Invalid operands, byte register can't be used here!"};
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

				throw std::runtime_error {"Invalid operands!"};
			}

			/// Increment
			void put_inc(Location dst) {

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

				throw std::runtime_error {"Invalid operands!"};
			}

			/// Decrement
			void put_dec(Location dst) {
				Registry dst_reg = dst.base;

				// short-form
				if (dst.is_simple() && dst.base.is_wide()) {

					if (dst.base.size == WORD) {
						put_inst_16bit_operand_mark();
					}

					put_byte((0b01001 << 3) | dst_reg.reg);
					return;
				}

				if (!dst_reg.is(UNSET)) {
					put_inst_std(0b111111, dst, 0b001, true, dst_reg.is_wide());
					return;
				}

				throw std::runtime_error {"Invalid operands!"};
			}

			/// Negate
			void put_neg(Location dst) {
				if (!dst.base.is(UNSET)) {
					put_inst_std(0b111101, dst, 0b011, true, dst.base.is_wide());
					return;
				}

				throw std::runtime_error {"Invalid operands!"};
			}

			/// Add
			void put_add(Location dst, Location src) {
				put_inst_tuple(dst, src, 0b000000, 0b000);
			}

			/// Add with carry
			void put_adc(Location dst, Location src) {
				put_inst_tuple(dst, src, 0b000100, 0b010);
			}

			/// Subtract
			void put_sub(Location dst, Location src) {
				put_inst_tuple(dst, src, 0b001010, 0b101);
			}

			/// Subtract with borrow
			void put_sbb(Location dst, Location src) {
				put_inst_tuple(dst, src, 0b000110, 0b011);
			}

			/// Compare
			void put_cmp(Location dst, Location src) {
				put_inst_tuple(dst, src, 0b001110, 0b111);
			}

			/// Binary And
			void put_and(Location dst, Location src) {
				put_inst_tuple(dst, src, 0b001000, 0b110);
			}

			/// Binary Or
			void put_or(Location dst, Location src) {
				put_inst_tuple(dst, src, 0b000010, 0b001);
			}

			/// Binary Xor
			void put_xor(Location dst, Location src) {
				put_inst_tuple(dst, src, 0b001100, 0b010);
			}

			/// Bit Test
			void put_bt(Location dst, Location src) {
				put_inst_btx(dst, src, 0b101000, 0b100);
			}

			/// Bit Test and Set
			void put_bts(Location dst, Location src) {
				put_inst_btx(dst, src, 0b101010, 0b101);
			}

			/// Bit Test and Reset
			void put_btr(Location dst, Location src) {
				put_inst_btx(dst, src, 0b101100, 0b110);
			}

			/// Bit Test and Complement
			void put_btc(Location dst, Location src) {
				put_inst_btx(dst, src, 0b101110, 0b111);
			}

			/// Multiply (Unsigned)
			void put_mul(Location dst) {
				if (dst.is_simple() || dst.reference) {
					put_inst_std(0b111101, dst, 0b100, true, dst.base.is_wide());
					return;
				}

				throw std::runtime_error {"Invalid operands!"};
			}

			/// Integer multiply (Signed)
			void put_imul(Location dst, Location src) {

				// TODO: This requires a change to the size system to work
				// short form
				// if (dst.is_simple() && src.is_memreg() && dst.base.name == Registry::Name::A) {
				// 	put_inst_std(0b111101, src, 0b101, true, dst.base.is_wide());
				// 	return;
				// }

				if (dst.is_simple() && src.is_memreg() && dst.base.size != BYTE) {
					put_inst_std(0b101011, src, dst.base.reg, true, true, true);
					return;
				}

				if (dst.is_simple() && src.is_immediate()) {
					put_imul(dst, dst, src);
					return;
				}

				throw std::runtime_error {"Invalid operands!"};
			}

			/// Integer multiply (Signed), triple arg version
			void put_imul(Location dst, Location src, Location val) {
				if (dst.is_simple() && src.is_memreg() && val.is_immediate() && dst.base.size != BYTE) {
					put_inst_std(0b011010, src, dst.base.reg, true /* TODO: sign flag */, true);

					// not sure why but it looks like IMUL uses 8bit immediate values
					put_byte(val.offset);
					return;
				}

				throw std::runtime_error {"Invalid operands!"};
			}

			/// Divide (Unsigned)
			void put_div(Location src) {
				if (src.is_memreg()) {
					put_inst_std(0b111101, src, 0b110, true, src.size != BYTE);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};
			}

			/// Integer divide (Signed)
			void put_idiv(Location src) {
				if (src.is_memreg()) {
					put_inst_std(0b111101, src, 0b111, true, src.size != BYTE);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};
			}

			/// Invert
			void put_not(Location dst) {
				if (dst.is_memreg()) {
					put_inst_std(0b111101, dst, 0b010, true, dst.base.is_wide());
					return;
				}

				throw std::runtime_error {"Invalid operand!"};
			}

			/// Rotate Left
			void put_rol(Location dst, Location src) {

				//       + - + ------- + - +
				// CF <- | M |   ...   | L | <- MB
				//       + - + ------- + - +
				//       |             |
				//       |             \_ Least Significant Bit (LB)
				//       \_ Most Significant Bit (MB)

				put_inst_shift(dst, src, INST_ROL);
			}

			/// Rotate Right
			void put_ror(Location dst, Location src) {

				//       + - + ------- + - +
				// LB -> | M |   ...   | L | -> CF
				//       + - + ------- + - +
				//       |             |
				//       |             \_ Least Significant Bit (LB)
				//       \_ Most Significant Bit (MB)

				put_inst_shift(dst, src, INST_ROR);
			}

			/// Rotate Left Through Carry
			void put_rcl(Location dst, Location src) {

				//       + - + ------- + - +
				// CF <- | M |   ...   | L | <- CF
				//       + - + ------- + - +
				//       |             |
				//       |             \_ Least Significant Bit (LB)
				//       \_ Most Significant Bit (MB)

				put_inst_shift(dst, src, INST_RCL);
			}

			/// Rotate Right Through Carry
			void put_rcr(Location dst, Location src) {

				//       + - + ------- + - +
				// CF -> | M |   ...   | L | -> CF
				//       + - + ------- + - +
				//       |             |
				//       |             \_ Least Significant Bit (LB)
				//       \_ Most Significant Bit (MB)

				put_inst_shift(dst, src, INST_RCR);
			}

			/// Shift Left
			void put_shl(Location dst, Location src) {

				//       + - + ------- + - +
				// CF <- | M |   ...   | L | <- 0
				//       + - + ------- + - +
				//       |             |
				//       |             \_ Least Significant Bit (LB)
				//       \_ Most Significant Bit (MB)

				put_inst_shift(dst, src, INST_SHL);
			}

			/// Shift Right
			void put_shr(Location dst, Location src) {

				//       + - + ------- + - +
				//  0 -> | M |   ...   | L | -> CF
				//       + - + ------- + - +
				//       |             |
				//       |             \_ Least Significant Bit (LB)
				//       \_ Most Significant Bit (MB)

				put_inst_shift(dst, src, INST_SHR);
			}

			/// Arithmetic Shift Left
			void put_sal(Location dst, Location src) {

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
			void put_sar(Location dst, Location src) {

				//       + - + ------- + - +
				// MB -> | M |   ...   | L | -> CF
				//       + - + ------- + - +
				//       |             |
				//       |             \_ Least Significant Bit (LB)
				//       \_ Most Significant Bit (MB)

				put_inst_shift(dst, src, INST_SAR);
			}

			/// Unconditional Jump
			void put_jmp(Location dst) {

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
					put_inst_std(0b111111, dst, 0b100, true, true);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};
			}

			/// Procedure Call
			void put_call(Location dst) {

				if (dst.is_labeled()) {

					// TODO shift
					put_byte(0b11101000);
					put_label(dst.label, DWORD);
					return;

				}

				if (dst.is_memreg()) {
					put_inst_std(0b111111, dst, 0b010, true, true);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};
			}

			/// Jump on Overflow
			Label put_jo(Label label) {
				put_inst_jx(label, 0b01110000, 0b10000000);
				return label;
			}

			/// Jump on Not Overflow
			Label put_jno(Label label) {
				put_inst_jx(label, 0b01110001, 0b10000001);
				return label;
			}

			/// Jump on Below
			Label put_jb(Label label) {
				put_inst_jx(label, 0b01110010, 0b10000010);
				return label;
			}

			/// Jump on Not Below
			Label put_jnb(Label label) {
				put_inst_jx(label, 0b01110011, 0b10000011);
				return label;
			}

			/// Jump on Equal
			Label put_je(Label label) {
				put_inst_jx(label, 0b01110100, 0b10000100);
				return label;
			}

			/// Jump on Not Equal
			Label put_jne(Label label) {
				put_inst_jx(label, 0b01110101, 0b10000101);
				return label;
			}

			/// Jump on Below or Equal
			Label put_jbe(Label label) {
				put_inst_jx(label, 0b01110110, 0b10000110);
				return label;
			}

			/// Jump on Not Below or Equal
			Label put_jnbe(Label label) {
				put_inst_jx(label, 0b01110111, 0b10000111);
				return label;
			}

			/// Jump on Sign
			Label put_js(Label label) {
				put_inst_jx(label, 0b01111000, 0b10001000);
				return label;
			}

			/// Jump on Not Sign
			Label put_jns(Label label) {
				put_inst_jx(label, 0b01111001, 0b10001001);
				return label;
			}

			/// Jump on Parity
			Label put_jp(Label label) {
				put_inst_jx(label, 0b01111010, 0b10001010);
				return label;
			}

			/// Jump on Not Parity
			Label put_jnp(Label label) {
				put_inst_jx(label, 0b01111011, 0b10001011);
				return label;
			}

			/// Jump on Less
			Label put_jl(Label label) {
				put_inst_jx(label, 0b01111100, 0b10001100);
				return label;
			}

			/// Jump on Not Less
			Label put_jnl(Label label) {
				put_inst_jx(label, 0b01111101, 0b10001101);
				return label;
			}

			/// Jump on Less or Equal
			Label put_jle(Label label) {
				put_inst_jx(label, 0b01111110, 0b10001110);
				return label;
			}

			/// Jump on Not Less or Equal
			Label put_jnle(Label label) {
				put_inst_jx(label, 0b01111111, 0b10001111);
				return label;
			}

			/// No Operation
			void put_nop() {
				put_byte(0b10010000);
			}

			/// Halt
			void put_hlt() {
				put_byte(0b11110100);
			}

			/// Wait
			void put_wait() {
				put_byte(0b10011011);
			}

			/// Push All
			void put_pusha() {
				put_byte(0b01100000);
			}

			/// Pop All
			void put_popa() {
				put_byte(0b01100001);
			}

			/// Push Flags
			void put_pushf() {
				put_byte(0b10011100);
			}

			/// Pop Flags
			void put_popf() {
				put_byte(0b10011101);
			}

			/// Clear Carry
			void put_clc() {
				put_byte(0b11111000);
			}

			/// Set Carry
			void put_stc() {
				put_byte(0b11111001);
			}

			/// Complement Carry Flag
			void put_cmc() {
				put_byte(0b11111000);
			}

			/// Store AH into flags
			void put_sahf() {
				put_byte(0b10011110);
			}

			/// Load status flags into AH register
			void put_lahf() {
				put_byte(0b10011111);
			}

			/// ASCII adjust for add
			void put_aaa() {
				put_byte(0b00110111);
			}

			/// Decimal adjust for add
			void put_daa() {
				put_byte(0b00111111);
			}

			/// ASCII adjust for subtract
			void put_aas() {
				put_byte(0b00100111);
			}

			/// Decimal adjust for subtract
			void put_das() {
				put_byte(0b00101111);
			}

			/// Alias to JB, Jump on Not Above or Equal
			Label put_jnae(Label label) {
				return put_jb(label);
			}

			/// Alias to JNB, Jump on Above or Equal
			Label put_jae(Label label) {
				return put_jnb(label);
			}

			/// Alias to JE, Jump on Zero
			Label put_jz(Label label) {
				return put_je(label);
			}

			/// Alias to JNE, Jump on Not Zero
			Label put_jnz(Label label) {
				return put_jne(label);
			}

			/// Alias to JBE, Jump on Not Above
			Label put_jna(Label label) {
				return put_jbe(label);
			}

			/// Alias to JNBE, Jump on Above
			Label put_ja(Label label) {
				return put_jnbe(label);
			}

			/// Alias to JP, Jump on Parity Even
			Label put_jpe(Label label) {
				return put_jp(label);
			}

			/// Alias to JNP, Jump on Parity Odd
			Label put_jpo(Label label) {
				return put_jnp(label);
			}

			/// Alias to JL, Jump on Not Greater or Equal
			Label put_jnge(Label label) {
				return put_jl(label);
			}

			/// Alias to JNL, Jump on Greater or Equal
			Label put_jge(Label label) {
				return put_jnl(label);
			}

			/// Alias to JLE, Jump on Not Greater
			Label put_jng(Label label) {
				return put_jle(label);
			}

			/// Alias to JNLE, Jump on Greater
			Label put_jg(Label label) {
				return put_jnle(label);
			}

			/// Jump on CX Zero
			Label put_jcxz(Label label) {
				put_inst_16bit_address_mark();
				return put_jecxz(label);
			}

			/// Jump on ECX Zero
			Label put_jecxz(Label label) {
				put_byte(0b11100011);
				put_label(label, BYTE);
				return label;
			}

			/// Loop Times
			Label put_loop(Label label) {
				put_byte(0b11100010);
				put_label(label, BYTE);
				return label;
			}

			/// ASCII adjust for division
			void put_aad() {

				// the operation performed by AAD:
				// AH = AL + AH * 10
				// AL = 0

				put_word(0b00001010'11010101);
			}

			/// ASCII adjust for multiplication
			void put_aam() {

				// the operation performed by AAM:
				// AH = AL div 10
				// AL = AL mod 10

				put_word(0b00001010'11010100);
			}

			/// Convert byte to word
			void put_cbw() {
				put_byte(0b10011000);
			}

			/// Convert word to double word
			void put_cwd() {
				put_byte(0b10011001);
			}

			/// Return from procedure
			void put_ret(uint16_t bytes = 0) {
				if (bytes != 0) {
					put_byte(0b11000010);
					put_word(bytes);
					return;
				}

				put_byte(0b11000011);
			}

			/// No Operation
			void put_fnop() {
				put_inst_fpu(0xD9, 0xD0);
			}

			/// Initialize FPU
			void put_finit() {
				put_wait();
				put_fninit();
			}

			/// Initialize FPU (without checking for pending unmasked exceptions)
			void put_fninit() {
				put_byte(0xDB);
				put_byte(0xE3);
			}

			/// Clear Exceptions
			void put_fclex() {
				put_wait();
				put_fnclex();
			}

			/// Clear Exceptions (without checking for pending unmasked exceptions)
			void put_fnclex() {
				put_inst_fpu(0xDB, 0xE2);
			}

			/// Store FPU State Word
			void put_fstsw(Location dst) {
				put_wait();
				put_fnstsw(dst);
			}

			/// Store FPU State Word (without checking for pending unmasked exceptions)
			void put_fnstsw(Location dst) {

				if (dst.is_simple() && dst.base.is(AX)) {
					put_inst_fpu(0xDF, 0xE0);
					return;
				}

				if (dst.is_memory() && dst.size == WORD) {
					put_inst_std(0xDD, dst, 7);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Store FPU Control Word
			void put_fstcw(Location dst) {
				put_wait();
				put_fnstcw(dst);
			}

			/// Store FPU Control Word (without checking for pending unmasked exceptions)
			void put_fnstcw(Location dst) {

				if (dst.is_memory() && dst.size == WORD) {
					put_inst_std(0xD9, dst, 7);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Load +1.0 Constant onto the stack
			void put_fld1() {
				put_inst_fpu(0xD9, 0xE8);
			}

			/// Load +0.0 Constant onto the stack
			void put_fld0() {
				put_inst_fpu(0xD9, 0xEE);
			}

			/// Load PI Constant onto the stack
			void put_fldpi() {
				put_inst_fpu(0xD9, 0xEB);
			}

			/// Load Log{2}(10) Constant onto the stack
			void put_fldl2t() {
				put_inst_fpu(0xD9, 0xE9);
			}

			/// Load Log{2}(e) Constant onto the stack
			void put_fldl2e() {
				put_inst_fpu(0xD9, 0xEA);
			}

			/// Load Log{10}(2) Constant onto the stack
			void put_fldlt2() {
				put_inst_fpu(0xD9, 0xEC);
			}

			/// Load Log{e}(2) Constant onto the stack
			void put_fldle2() {
				put_inst_fpu(0xD9, 0xED);
			}

			/// Load x87 FPU Control Word
			void put_fldcw(Location src) {

				// fldcw src:m2byte
				if (src.is_memory() && src.size == WORD) {
					put_inst_std(0xD9, src, 5);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Compute 2^x - 1
			void put_f2xm1() {
				put_inst_fpu(0xD9, 0xF0);
			}

			/// Absolute Value
			void put_fabs() {
				put_inst_fpu(0xD9, 0xE1);
			}

			/// Change Sign
			void put_fchs() {
				put_inst_fpu(0xD9, 0xE0);
			}

			/// Compute Cosine
			void put_fcos() {
				put_inst_fpu(0xD9, 0xFF);
			}

			/// Compute Sine
			void put_fsin() {
				put_inst_fpu(0xD9, 0xFE);
			}

			/// Compute Sine and Cosine, sets ST(0) to sin(ST(0)), and pushes cos(ST(0)) onto the stack
			void put_fsincos() {
				put_inst_fpu(0xD9, 0xFB);
			}

			/// Decrement Stack Pointer
			void put_fdecstp() {
				put_inst_fpu(0xD9, 0xF6);
			}

			/// Increment Stack Pointer
			void put_fincstp() {
				put_inst_fpu(0xD9, 0xF7);
			}

			/// Partial Arctangent, sets ST(1) to arctan(ST(1) / ST(0)), and pops the stack
			void put_fpatan() {
				put_inst_fpu(0xD9, 0xF3);
			}

			/// Partial Remainder, sets ST(0) to ST(0) % ST(1)
			void put_fprem() {
				put_inst_fpu(0xD9, 0xF8);
			}

			/// Partial IEEE Remainder, sets ST(0) to ST(0) % ST(1)
			void put_fprem1() {
				put_inst_fpu(0xD9, 0xF5);
			}

			/// Partial Tangent, sets ST(0) to tan(ST(0)), and pushes 1 onto the stack
			void put_fptan() {
				put_inst_fpu(0xD9, 0xF2);
			}

			/// Round to Integer, Rounds ST(0) to an integer
			void put_frndint() {
				put_inst_fpu(0xD9, 0xFC);
			}

			/// Scale, ST(0) by ST(1), Sets ST(0) to ST(0)*2^floor(ST(1))
			void put_fscale() {
				put_inst_fpu(0xD9, 0xFD);
			}

			/// Square Root, sets ST(0) to sqrt(ST(0))
			void put_fsqrt() {
				put_inst_fpu(0xD9, 0xFA);
			}

			/// Load Floating-Point Value
			void put_fld(Location src) {

				// fld src:m32fp
				if (src.is_memory() && src.size == DWORD) {
					put_inst_std(0xD9, src, 0);
					return;
				}

				// fld src:m64fp
				if (src.is_memory() && src.size == QWORD) {
					put_inst_std(0xDD, src, 0);
					return;
				}

				// fld src:m80fp
				if (src.is_memory() && src.size == TWORD) {
					put_inst_std(0xDB, src, 5);
					return;
				}

				// fld src:st(i)
				if (src.is_floating()) {
					put_inst_fpu(0xD9, 0xC0, src.offset);
					return;
				}

				if (src.is_memory()) {
					throw std::runtime_error {"Invalid operand size!"};
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Load Integer Value
			void put_fild(Location src) {

				// fild src:m16int
				if (src.is_memory() && src.size == WORD) {
					put_inst_std(0xDF, src, 0);
					return;
				}

				// fild src:m32int
				if (src.is_memory() && src.size == DWORD) {
					put_inst_std(0xDB, src, 0);
					return;
				}

				// fild src:m64int
				if (src.is_memory() && src.size == QWORD) {
					put_inst_std(0xDF, src, 5);
					return;
				}

				if (src.is_memory()) {
					throw std::runtime_error {"Invalid operand size!"};
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Store Floating-Point Value
			void put_fst(Location dst) {

				// fst dst:m32fp
				if (dst.is_memory() && dst.size == DWORD) {
					put_inst_std(0xD9, dst, 2);
					return;
				}

				// fst dst:m64fp
				if (dst.is_memory() && dst.size == QWORD) {
					put_inst_std(0xDD, dst, 2);
					return;
				}

				// fst src:st(i)
				if (dst.is_floating()) {
					put_inst_fpu(0xDD, 0xD0, dst.offset);
					return;
				}

				if (dst.is_memory()) {
					throw std::runtime_error {"Invalid operand size!"};
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Store Floating-Point Value and Pop
			void put_fstp(Location dst) {

				// fstp dst:m32fp
				if (dst.is_memory() && dst.size == DWORD) {
					put_inst_std(0xD9, dst, 3);
					return;
				}

				// fstp dst:m64fp
				if (dst.is_memory() && dst.size == QWORD) {
					put_inst_std(0xDD, dst, 3);
					return;
				}

				// fstp dst:m80fp
				if (dst.is_memory() && dst.size == TWORD) {
					put_inst_std(0xDB, dst, 7);
					return;
				}

				// fstp src:st(i)
				if (dst.is_floating()) {
					put_inst_fpu(0xDD, 0xD8, dst.offset);
					return;
				}

				if (dst.is_memory()) {
					throw std::runtime_error {"Invalid operand size!"};
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Store Floating-Point Value As Integer
			void put_fist(Location dst) {

				// fist src:m16int
				if (dst.is_memory() && dst.size == WORD) {
					put_inst_std(0xDF, dst, 2);
					return;
				}

				// fist src:m32int
				if (dst.is_memory() && dst.size == DWORD) {
					put_inst_std(0xDB, dst, 2);
					return;
				}

				if (dst.is_memory()) {
					throw std::runtime_error {"Invalid operand size!"};
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Store Floating-Point Value As Integer And Pop
			void put_fistp(Location dst) {

				// fist src:m16int
				if (dst.is_memory() && dst.size == WORD) {
					put_inst_std(0xDF, dst, 3);
					return;
				}

				// fist src:m32int
				if (dst.is_memory() && dst.size == DWORD) {
					put_inst_std(0xDB, dst, 3);
					return;
				}

				// fist src:m64int
				if (dst.is_memory() && dst.size == QWORD) {
					put_inst_std(0xDF, dst, 7);
					return;
				}

				if (dst.is_memory()) {
					throw std::runtime_error {"Invalid operand size!"};
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Store Floating-Point Value As Integer With Truncation And Pop
			void put_fisttp(Location dst) {

				// fist src:m16int
				if (dst.is_memory() && dst.size == WORD) {
					put_inst_std(0xDF, dst, 1);
					return;
				}

				// fist src:m32int
				if (dst.is_memory() && dst.size == DWORD) {
					put_inst_std(0xDB, dst, 1);
					return;
				}

				// fist src:m64int
				if (dst.is_memory() && dst.size == QWORD) {
					put_inst_std(0xDD, dst, 1);
					return;
				}

				if (dst.is_memory()) {
					throw std::runtime_error {"Invalid operand size!"};
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Free Floating-Point Register
			void put_ffree(Location src) {

				if (src.is_floating()) {
					put_inst_fpu(0xDD, 0xC0, src.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Move SRC, if below, into ST+0
			void put_fcmovb(Location src) {

				if (src.is_floating()) {
					put_inst_fpu(0xDA, 0xC0, src.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Move SRC, if equal, into ST+0
			void put_fcmove(Location src) {

				if (src.is_floating()) {
					put_inst_fpu(0xDA, 0xC8, src.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Move SRC, if below or equal, into ST+0
			void put_fcmovbe(Location src) {

				if (src.is_floating()) {
					put_inst_fpu(0xDA, 0xD0, src.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Move SRC, if unordered with, into ST+0
			void put_fcmovu(Location src) {

				if (src.is_floating()) {
					put_inst_fpu(0xDA, 0xD8, src.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Move SRC, if not below, into ST+0
			void put_fcmovnb(Location src) {

				if (src.is_floating()) {
					put_inst_fpu(0xDB, 0xC0, src.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Move SRC, if not equal, into ST+0
			void put_fcmovne(Location src) {

				if (src.is_floating()) {
					put_inst_fpu(0xDB, 0xC8, src.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Move SRC, if not below or equal, into ST+0
			void put_fcmovnbe(Location src) {

				if (src.is_floating()) {
					put_inst_fpu(0xDB, 0xD0, src.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Move SRC, if not unordered with, into ST+0
			void put_fcmovnu(Location src) {

				if (src.is_floating()) {
					put_inst_fpu(0xDB, 0xD8, src.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Compare ST+0 with SRC
			void put_fcom(Location src) {

				// fcom src:m32fp
				if (src.is_memory() && src.size == DWORD) {
					put_inst_std(0xD8, src, 2);
					return;
				}

				// fcom src:m64fp
				if (src.is_memory() && src.size == QWORD) {
					put_inst_std(0xDC, src, 2);
					return;
				}

				// fcom st(0), src:st(i)
				if (src.is_floating()) {
					put_inst_fpu(0xD8, 0xD0, src.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Compare ST+0 with SRC And Pop
			void put_fcomp(Location src) {

				// fcomp src:m32fp
				if (src.is_memory() && src.size == DWORD) {
					put_inst_std(0xD8, src, 3);
					return;
				}

				// fcomp src:m64fp
				if (src.is_memory() && src.size == QWORD) {
					put_inst_std(0xDC, src, 3);
					return;
				}

				// fcomp st(0), src:st(i)
				if (src.is_floating()) {
					put_inst_fpu(0xD8, 0xD8, src.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Compare ST+0 with ST+1 And Pop Both
			void put_fcompp() {
				put_inst_fpu(0xDE, 0xD9);
			}

			/// Compare ST+0 with Integer SRC
			void put_ficom(Location src) {

				// ficom src:m16int
				if (src.is_memory() && src.size == WORD) {
					put_inst_std(0xDE, src, 2);
					return;
				}

				// ficom src:m32int
				if (src.is_memory() && src.size == DWORD) {
					put_inst_std(0xDA, src, 2);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Compare ST+0 with Integer SRC, And Pop
			void put_ficomp(Location src) {

				// ficom src:m16int
				if (src.is_memory() && src.size == WORD) {
					put_inst_std(0xDE, src, 3);
					return;
				}

				// ficom src:m32int
				if (src.is_memory() && src.size == DWORD) {
					put_inst_std(0xDA, src, 3);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Compare ST+0 with SRC and set EFLAGS
			void put_fcomi(Location src) {

				// fcomi st(0), src:st(i)
				if (src.is_floating()) {
					put_inst_fpu(0xDB, 0xF0, src.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Compare ST+0 with SRC, Pop, and set EFLAGS
			void put_fcomip(Location src) {

				// fcomip st(0), src:st(i)
				if (src.is_floating()) {
					put_inst_fpu(0xDF, 0xF0, src.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Compare, and check for ordered values, ST+0 with SRC and set EFLAGS
			void put_fucomi(Location src) {

				// fcomi st(0), src:st(i)
				if (src.is_floating()) {
					put_inst_fpu(0xDB, 0xF8, src.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Compare, and check for ordered values, ST+0 with SRC, Pop, and set EFLAGS
			void put_fucomip(Location src) {

				// fcomip st(0), src:st(i)
				if (src.is_floating()) {
					put_inst_fpu(0xDF, 0xF8, src.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Multiply By Memory Float
			void put_fmul(Location src) {

				// fmul src:m32fp
				if (src.is_memory() && src.size == DWORD) {
					put_inst_std(0xD8, src, 1);
					return;
				}

				// fmul src:m64fp
				if (src.is_memory() && src.size == QWORD) {
					put_inst_std(0xDC, src, 1);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Multiply By Memory Integer
			void put_fimul(Location src) {

				// fmul src:m32int
				if (src.is_memory() && src.size == DWORD) {
					put_inst_std(0xDA, src, 1);
					return;
				}

				// fmul src:m16int
				if (src.is_memory() && src.size == WORD) {
					put_inst_std(0xDE, src, 1);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Multiply
			void put_fmul(Location dst, Location src) {

				// fmul dst:st(0), src:st(i)
				if (dst.is_st0() && src.is_floating()) {
					put_inst_fpu(0xD8, 0xC8, src.offset);
					return;
				}

				// fmul dst:st(i), src:st(0)
				if (dst.is_floating() && src.is_st0()) {
					put_inst_fpu(0xDC, 0xC8, dst.offset);
					return;
				}

				throw std::runtime_error {"Invalid operands!"};

			}

			/// Multiply And Pop
			void put_fmulp(Location dst) {

				// fmulp dst:st(i), st(0)
				if (dst.is_floating()) {
					put_inst_fpu(0xDE, 0xC8, dst.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Add Memory Float
			void put_fadd(Location src) {

				// fadd src:m32fp
				if (src.is_memory() && src.size == DWORD) {
					put_inst_std(0xD8, src, 0);
					return;
				}

				// fadd src:m64fp
				if (src.is_memory() && src.size == QWORD) {
					put_inst_std(0xDC, src, 0);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Add Memory Integer
			void put_fiadd(Location src) {

				// fadd src:m32int
				if (src.is_memory() && src.size == DWORD) {
					put_inst_std(0xDA, src, 0);
					return;
				}

				// fadd src:m16int
				if (src.is_memory() && src.size == WORD) {
					put_inst_std(0xDE, src, 0);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Add
			void put_fadd(Location dst, Location src) {

				// fadd dst:st(0), src:st(i)
				if (dst.is_st0() && src.is_floating()) {
					put_inst_fpu(0xD8, 0xC0, src.offset);
					return;
				}

				// fadd dst:st(i), src:st(0)
				if (dst.is_floating() && src.is_st0()) {
					put_inst_fpu(0xDC, 0xC0, dst.offset);
					return;
				}

				throw std::runtime_error {"Invalid operands!"};

			}

			/// Add And Pop
			void put_faddp(Location dst) {

				// faddp dst:st(i), st(0)
				if (dst.is_floating()) {
					put_inst_fpu(0xDE, 0xC0, dst.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Divide By Memory Float
			void put_fdiv(Location src) {

				// fdiv src:m32fp
				if (src.is_memory() && src.size == DWORD) {
					put_inst_std(0xD8, src, 6);
					return;
				}

				// fdiv src:m64fp
				if (src.is_memory() && src.size == QWORD) {
					put_inst_std(0xDC, src, 6);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Divide By Memory Integer
			void put_fidiv(Location src) {

				// fidiv src:m32int
				if (src.is_memory() && src.size == DWORD) {
					put_inst_std(0xDA, src, 6);
					return;
				}

				// fidiv src:m16int
				if (src.is_memory() && src.size == WORD) {
					put_inst_std(0xDE, src, 6);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Divide
			void put_fdiv(Location dst, Location src) {

				// fdiv dst:st(0), src:st(i)
				if (dst.is_st0() && src.is_floating()) {
					put_inst_fpu(0xD8, 0xF0, src.offset);
					return;
				}

				// fdiv dst:st(i), src:st(0)
				if (dst.is_floating() && src.is_st0()) {
					put_inst_fpu(0xDC, 0xF8, dst.offset);
					return;
				}

				throw std::runtime_error {"Invalid operands!"};

			}

			/// Divide And Pop
			void put_fdivp(Location dst) {

				// fdivp dst:st(i), st(0)
				if (dst.is_floating()) {
					put_inst_fpu(0xDE, 0xF8, dst.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Divide Memory Float
			void put_fdivr(Location src) {

				// fdivr src:m32fp
				if (src.is_memory() && src.size == DWORD) {
					put_inst_std(0xD8, src, 7);
					return;
				}

				// fdivr src:m64fp
				if (src.is_memory() && src.size == QWORD) {
					put_inst_std(0xDC, src, 7);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Divide Memory Integer
			void put_fidivr(Location src) {

				// fidivr src:m32int
				if (src.is_memory() && src.size == DWORD) {
					put_inst_std(0xDA, src, 7);
					return;
				}

				// fidivr src:m16int
				if (src.is_memory() && src.size == WORD) {
					put_inst_std(0xDE, src, 7);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			/// Reverse Divide
			void put_fdivr(Location dst, Location src) {

				// fdivr dst:st(0), src:st(i)
				if (dst.is_st0() && src.is_floating()) {
					put_inst_fpu(0xD8, 0xF8, src.offset);
					return;
				}

				// fdivr dst:st(i), src:st(0)
				if (dst.is_floating() && src.is_st0()) {
					put_inst_fpu(0xDC, 0xF0, dst.offset);
					return;
				}

				throw std::runtime_error {"Invalid operands!"};

			}

			/// Reverse Divide And Pop
			void put_fdivrp(Location dst) {

				// fdivrp dst:st(i), st(0)
				if (dst.is_floating()) {
					put_inst_fpu(0xDE, 0xF0, dst.offset);
					return;
				}

				throw std::runtime_error {"Invalid operand!"};

			}

			void put_byte(uint8_t byte) {
				buffer.push_back(byte);
			}

			void put_word(uint16_t word) {
				const uint8_t* imm_ptr = (uint8_t*) &word;

				buffer.push_back(imm_ptr[0]);
				buffer.push_back(imm_ptr[1]);
			}

			void put_dword(uint32_t dword) {
				const uint8_t* imm_ptr = (uint8_t*) &dword;

				buffer.push_back(imm_ptr[0]);
				buffer.push_back(imm_ptr[1]);
				buffer.push_back(imm_ptr[2]);
				buffer.push_back(imm_ptr[3]);
			}

			void put_dword_f(float dword) {
				const uint8_t* imm_ptr = (uint8_t*) &dword;

				buffer.push_back(imm_ptr[0]);
				buffer.push_back(imm_ptr[1]);
				buffer.push_back(imm_ptr[2]);
				buffer.push_back(imm_ptr[3]);
			}

			void put_qword(uint64_t dword) {
				const uint8_t* imm_ptr = (uint8_t*) &dword;

				buffer.push_back(imm_ptr[0]);
				buffer.push_back(imm_ptr[1]);
				buffer.push_back(imm_ptr[2]);
				buffer.push_back(imm_ptr[3]);
				buffer.push_back(imm_ptr[4]);
				buffer.push_back(imm_ptr[5]);
				buffer.push_back(imm_ptr[6]);
				buffer.push_back(imm_ptr[7]);
			}

			void put_qword_f(double dword) {
				const uint8_t* imm_ptr = (uint8_t*) &dword;

				buffer.push_back(imm_ptr[0]);
				buffer.push_back(imm_ptr[1]);
				buffer.push_back(imm_ptr[2]);
				buffer.push_back(imm_ptr[3]);
				buffer.push_back(imm_ptr[4]);
				buffer.push_back(imm_ptr[5]);
				buffer.push_back(imm_ptr[6]);
				buffer.push_back(imm_ptr[7]);
			}

			ExecutableBuffer bake(bool debug = false) {

				ExecutableBuffer result {buffer.size(), labels};
				size_t absolute = result.get_address();

				for (LabelCommand command : commands) {
					const long offset = command.relative ? command.offset + command.size : (-absolute);
					const long imm_val = get_label(command.label) - offset + command.shift;
					const uint8_t* imm_ptr = (uint8_t*) &imm_val;
					memcpy(buffer.data() + command.offset, imm_ptr, command.size);
				}

				result.write(buffer);

				#if DEBUG_MODE
				debug = true;
				#endif

				if (debug) {
					int i = 0;

					for (uint8_t byte : buffer) {
						std::bitset<8> bin(byte);
						std::cout << std::setfill ('0') << std::setw(4) << i << " | " << std::setfill ('0') << std::setw(2) << std::hex << ((int) byte) << ' ' << bin << std::endl;
						i ++;
					}

					std::cout << "./unasm.sh \"db ";
					bool first = true;

					for (uint8_t byte: buffer) {
						if (!first) {
							std::cout << ", ";
						}

						first = false;
						std::cout << '0' << std::setfill('0') << std::setw(2) << std::hex << ((int) byte) << "h";
					}

					std::cout << '"' << std::endl;
				}

				return result;
			}

	};

}