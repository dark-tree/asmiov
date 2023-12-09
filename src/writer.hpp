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
				put_byte(opcode << 2 | (d << 1) | w);
			}

			void put_inst_w(uint8_t opcode, bool w) {
				put_byte(opcode << 1 | w);
			}

			void put_inst_mod_reg_rm(uint8_t mod, uint8_t reg, uint8_t r_m) {
				put_byte(r_m | (reg << 3) | (mod << 6));
			}

			void put_inst_sib(uint8_t ss, uint8_t index, uint8_t base) {
				put_byte(base | (index << 3) | (ss << 6));
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

				// we have to use the SIB byte to target ESP
				bool sib = dst.base.is(ESP) || dst.is_indexed();
				uint8_t mod = dst.get_mod_flag();

				// special case for [EBP + (any indexed) + 0]
				if (dst_reg.is(EBP)) {
					mod = MOD_BYTE;
				}

				// TODO test
				if (dst_reg.size == WORD) {
					put_inst_16bit_mark();
				}

				put_inst_dw(opcode, direction, dst_reg.wide);
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

				put_inst_std(src.is_immediate() ? 0b110001 : 0b100010, dst, src_reg, direction);

				if (src.is_immediate()) {
					put_dword(src.offset);
				}
			}

			void put_inst_16bit_mark() {
				put_byte(0b01100110);
			}

		public:

			void put_mov(Location dst, Location src) {

				// mov REG, REG
				if (src.is_simple() && dst.is_simple()) {
					Registry dst_reg = dst.base;
					Registry src_reg = src.base;

					if (src_reg.size != dst_reg.size) {
						throw std::runtime_error {"Operands must have equal size!"};
					}

					put_inst_dw(0b100010, true, dst_reg.wide);
					put_inst_mod_reg_rm(MOD_SHORT, dst_reg.reg, src_reg.reg);
					return;
				}

				// mov REG, VAL
				if (src.is_immediate() && dst.is_simple()) {
					Registry dst_reg = dst.base;
					uint32_t src_val = src.offset;

					put_byte((0b1011 << 4) | (dst_reg.wide << 3) | dst_reg.reg);
					put_dword(src_val);
					return;
				}

				// mov REG, [REG]
				// mov REG, [REG + VAL]
				// mov REG, [REG + REG * VAL + VAL]
				if (dst.is_simple() && src.reference && !src.base.is(UNSET)) {
					put_inst_mov(src, dst, true);

					return;
				}

				// mov [REG], VAL/REG
				// mov [REG + VAL], VAL/REG
				// mov [REG + REG * VAL + VAL], VAL/REG
				if ((src.is_immediate() || src.is_simple()) && dst.reference && !dst.base.is(UNSET)) {
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

				if (dst.is_simple() || (dst.reference && !dst.base.is(UNSET))) {
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

				if (dst.is_simple() || (dst.reference && !dst.base.is(UNSET))) {
					put_inst_std(0b111111, dst, 0b001, true);
					return;
				}

				throw std::runtime_error {"Invalid operands!"};
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

			void put_ret(int bytes = 0) {
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

				return ExecutableBuffer {buffer};
			}

	};

}