#pragma once

#include <bitset>
#include <iomanip>
#include <utility>
#include "address.hpp"
#include "buffer.hpp"

namespace asmio::x86 {

	class BufferWriter {

		private:

			std::vector<uint8_t> buffer;

			void put_inst_dw(uint8_t opcode, bool d, bool w) {

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

				put_byte(opcode << 2 | (d << 1) | w);
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

				// Index register can be omitted from SIB by setting index' to NO_SIB_INDEX and
				// 'ss' to NO_SIB_SCALE, this is useful for encoding the ESP registry

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

			void put_inst_std(uint8_t opcode, Location dst, uint8_t reg, bool direction) {
				Registry dst_reg = dst.base;

				// TODO test
				if (dst_reg.size == WORD) {
					put_inst_16bit_mark();
				}

				if (dst.is_simple()) {
					put_inst_dw(opcode, direction, dst_reg.is_wide());
					put_inst_mod_reg_rm(MOD_SHORT, reg, dst_reg.reg);
					return;
				}

				// we have to use the SIB byte to target ESP
				bool sib = dst.base.is(ESP) || dst.is_indexed();
				uint8_t mod = dst.get_mod_flag();

				// special case for [EBP + (any indexed) + 0]
				if (dst_reg.is(EBP)) {
					mod = MOD_BYTE;
				}

				put_inst_dw(opcode, direction, dst_reg.is_wide());
				put_inst_mod_reg_rm(mod, reg, sib ? RM_SIB : dst_reg.reg);

				if (sib) {
					put_inst_sib(dst);
				}

				if (mod == MOD_BYTE) {
					put_byte(dst.offset);
				}

				if (mod == MOD_QUAD) {
					put_dword(dst.offset);
				}
			}

			void put_inst_mov(Location dst, Location src, bool direction) {

				// for immediate values this will equal 0
				uint8_t src_reg = src.base.reg;
				uint8_t dst_len = dst.base.size;

				put_inst_std(src.is_immediate() ? 0b110001 : 0b100010, dst, src_reg, direction);

				if (src.is_immediate()) {
					put_inst_imm(src.offset, dst_len);
				}
			}

			void put_inst_shift(Location dst, Location src, uint8_t inst) {

				if (src.is_simple() && src.base.is(CL)) {
					put_inst_std(0b110100, dst, inst, true);

					return;
				}

				if (src.is_immediate()) {
					uint8_t src_val = src.offset;

					if (src_val == 1) {
						put_inst_std(0b110100, dst, inst, false);
					} else {
						put_inst_std(0b110000, dst, inst, false);
						put_byte(src_val);
					}

					return;
				}

				throw std::runtime_error {"Invalid operands!"};

			}

			void put_inst_16bit_mark() {
				put_byte(0b01100110);
			}

		public:

			void put_mov(Location dst, Location src) {

				// verify operand size
				if (src.is_simple() && dst.is_simple()) {
					if (src.base.size != dst.base.size) {
						throw std::runtime_error {"Operands must have equal size!"};
					}
				}

				// mov REG, VAL
				if (src.is_immediate() && dst.is_simple()) {
					Registry dst_reg = dst.base;
					uint32_t src_val = src.offset;

					if (dst.base.size == WORD) {
						put_inst_16bit_mark();
					}

					put_byte((0b1011 << 4) | (dst_reg.is_wide() << 3) | dst_reg.reg);
					put_inst_imm(src_val, dst.base.size);
					return;
				}

				// mov REG, REG
				// mov REG, [REG]
				// mov REG, [REG + VAL]
				// mov REG, [REG + REG * VAL + VAL]
				if (dst.is_simple() && !src.base.is(UNSET)) {
					put_inst_mov(src, dst, true);

					return;
				}

				// mov [REG], VAL/REG
				// mov [REG + VAL], VAL/REG
				// mov [REG + REG * VAL + VAL], VAL/REG
				if ((src.is_immediate() || src.is_simple()) && !dst.base.is(UNSET)) {
					put_inst_mov(dst, src, src.is_immediate());

					return;
				}

				throw std::runtime_error {"Invalid operands!"};
			}

			void put_inc(Location dst) {

				// inc REG
				if (dst.is_simple()) {
					Registry dst_reg = dst.base;

					if (dst.base.size == WORD) {
						put_inst_16bit_mark();
					}

					put_byte((0b01000 << 3) | dst_reg.reg);
					return;
				}

				// inc [REG]
				// inc [REG + VAL]
				// inc [REG + REG * VAL + VAL]
				if (dst.reference && !dst.base.is(UNSET)) {
					put_inst_std(0b111111, dst, 0b000, true);
					return;
				}

				throw std::runtime_error {"Invalid operands!"};
			}

			void put_dec(Location dst) {

				// dec REG
				if (dst.is_simple()) {
					Registry dst_reg = dst.base;

					if (dst.base.size == WORD) {
						put_inst_16bit_mark();
					}

					put_byte((0b01001 << 3) | dst_reg.reg);
					return;
				}

				// dec [REG]
				// dec [REG + VAL]
				// dec [REG + REG * VAL + VAL]
				if (dst.reference && !dst.base.is(UNSET)) {
					put_inst_std(0b111111, dst, 0b001, true);
					return;
				}

				throw std::runtime_error {"Invalid operands!"};
			}

			void put_neg(Location dst) {

				// neg REG
				// neg [REG]
				// neg [REG + VAL]
				// neg [REG + REG * VAL + VAL]
				if (!dst.base.is(UNSET)) {
					put_inst_std(0b111101, dst, 0b011, true);
					return;
				}

				throw std::runtime_error {"Invalid operands!"};
			}

			void put_rol(Location dst, Location src) {
				put_inst_shift(dst, src, INST_ROL);
			}

			void put_ror(Location dst, Location src) {
				put_inst_shift(dst, src, INST_ROR);
			}

			void put_rcl(Location dst, Location src) {
				put_inst_shift(dst, src, INST_RCL);
			}

			void put_rcr(Location dst, Location src) {
				put_inst_shift(dst, src, INST_RCR);
			}

			void put_shl(Location dst, Location src) {
				put_inst_shift(dst, src, INST_SHL);
			}

			void put_shr(Location dst, Location src) {
				put_inst_shift(dst, src, INST_SHR);
			}

			void put_sal(Location dst, Location src) {
				put_inst_shift(dst, src, INST_SHL); // SAL == SHL
			}

			void put_sar(Location dst, Location src) {
				put_inst_shift(dst, src, INST_SAR);
			}

			void put_nop() {
				put_byte(0b10010000);
			}

			void put_pusha() {
				put_byte(0b01100000);
			}

			void put_popa() {
				put_byte(0b01100001);
			}

			void put_pushf() {
				put_byte(0b10011100);
			}

			void put_popf() {
				put_byte(0b10011101);
			}

			void put_clc() {
				put_byte(0b11111000);
			}

			void put_stc() {
				put_byte(0b11111001);
			}

			void put_cmc() {
				put_byte(0b11111000);
			}

			void put_ret(uint16_t bytes = 0) {
				if (bytes != 0) {
					put_byte(0b11000010);
					put_word(bytes);
					return;
				}

				put_byte(0b11000011);
			}

			void put_byte(uint8_t byte) {
				buffer.push_back(byte);
			}

			void put_word(uint16_t word) {
				buffer.push_back((word & 0x000000FF) >> 8 * 0);
				buffer.push_back((word & 0x0000FF00) >> 8 * 1);
			}

			void put_dword(uint32_t dword) {
				buffer.push_back((dword & 0x000000FF) >> 8 * 0);
				buffer.push_back((dword & 0x0000FF00) >> 8 * 1);
				buffer.push_back((dword & 0x00FF0000) >> 8 * 2);
				buffer.push_back((dword & 0xFF000000) >> 8 * 3);
			}

			ExecutableBuffer bake() {

				#if DEBUG_MODE
				int i = 0;

				for (uint8_t byte : buffer) {
					std::bitset<8> bin(byte);
					std::cout << std::setfill ('0') << std::setw(4) << i << " | " << std::setfill ('0') << std::setw(2) << std::hex << ((int) byte) << ' ' << bin << std::endl;
					i ++;
				}

				std::cout << "db ";

				for (uint8_t byte : buffer) {
					std::cout << '0' << std::setfill ('0') << std::setw(2) << std::hex << ((int) byte) << "h, ";
				}

				std::cout << std::endl;
				#endif

				return ExecutableBuffer {buffer};
			}

	};

}