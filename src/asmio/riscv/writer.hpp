#pragma once

#include <asmio/program/sizes.hpp>
#include <asmio/program/writer.hpp>
#include <asmio/riscv/argument/registry.hpp>
#include <asmio/riscv/argument/condition.hpp>
#include <asmio/riscv/argument/order.hpp>

namespace asmio::riscv {

	class BufferWriter : public BasicBufferWriter {

		protected:

			void put_inst_r(uint8_t func7, Registry rs2, Registry rs1, uint8_t func3, Registry rd, uint8_t opc7);
			void put_inst_i(uint16_t imm12, Registry rs1, uint8_t func3, Registry rd, uint8_t opc7);
			void put_inst_s(uint16_t imm12, Registry rs2, Registry rs1, uint8_t func3, uint8_t opc7);
			void put_inst_b(uint16_t imm12, Registry rs2, Registry rs1, uint8_t func3, uint8_t opc7);
			void put_inst_u(uint32_t imm20, Registry rd, uint8_t opc7);
			void put_inst_j(uint32_t imm20, Registry rd, uint8_t opc7);
			void put_inst_a(uint8_t func5, Registry rs2, Registry rs1, Size size, Registry rd, Order order);

		public:

			BufferWriter(SegmentedBuffer& buffer);

			INST put_add(Registry rd, Registry rs, int16_t imm12); ///< Add signed 12 bit immediate to rs and save result to rd
			INST put_xor(Registry rd, Registry rs, int16_t imm12); ///< Bitwise XOR 12 bit immediate rs and save result to rd
			INST put_or(Registry rd, Registry rs, int16_t imm12); ///< Bitwise OR 12 bit immediate rs and save result to rd
			INST put_and(Registry rd, Registry rs, int16_t imm12); ///< Bitwise AND 12 bit immediate rs and save result to rd
			INST put_sll(Registry rd, Registry rs, int16_t imm5); ///< Shift register rs left by a 12 bit immediate and save result to rd
			INST put_srl(Registry rd, Registry rs, int16_t imm5); ///< Shift register rs right logically by a 12 bit immediate and save result to rd
			INST put_sra(Registry rd, Registry rs, int16_t imm5); ///< Shift register rs right arythetically by a 12 bit immediate and save result to rd
			INST put_slt(Registry rd, Registry rs, int16_t imm12); ///< Set rd to 1 if rs is less than a 12 bit immediate, and to 0 otherwise (signed)
			INST put_sltu(Registry rd, Registry rs, int16_t imm12); ///< Set rd to 1 if rs1 is less than a 12 bit immediate, and to 0 otherwise (unsigned)
			INST put_add(Registry rd, Registry rs1, Registry rs2); ///< Add register rs1 to rs2 and save result to rd
			INST put_sub(Registry rd, Registry rs1, Registry rs2); ///< Subtract register rs1 from rs2 and save result to rd
			INST put_xor(Registry rd, Registry rs1, Registry rs2); ///< Bitwise XOR register rs1 with rs2 and save result to rd
			INST put_or(Registry rd, Registry rs1, Registry rs2); ///< Bitwise OR register rs1 with rs2 and save result to rd
			INST put_and(Registry rd, Registry rs1, Registry rs2); ///< Bitwise AND register rs1 with rs2 and save result to rd
			INST put_sll(Registry rd, Registry rs1, Registry rs2); ///< Shift register rs1 left by rs2 and save result to rd
			INST put_srl(Registry rd, Registry rs1, Registry rs2); ///< Shift register rs1 right logically by rs2 and save result to rd
			INST put_sra(Registry rd, Registry rs1, Registry rs2); ///< Shift register rs1 right arythetically by rs2 and save result to rd
			INST put_slt(Registry rd, Registry rs1, Registry rs2); ///< Set rd to 1 if rs1 is less than rs2, and to 0 otherwise (signed)
			INST put_sltu(Registry rd, Registry rs1, Registry rs2); ///< Set rd to 1 if rs1 is less than rs2, and to 0 otherwise (unsigned)
			INST put_lb(Registry rd, Registry rs, int16_t imm12 = 0); ///< Load byte at rs + imm12 into rd (with sign extension)
			INST put_lw(Registry rd, Registry rs, int16_t imm12 = 0); ///< Load word at rs + imm12 into rd (with sign extension)
			INST put_ld(Registry rd, Registry rs, int16_t imm12 = 0); ///< Load dword at rs + imm12 into rd (with sign extension)
			INST put_lbu(Registry rd, Registry rs, int16_t imm12 = 0); ///< Load byte at rs + imm12 into rd (with zero extension)
			INST put_lwu(Registry rd, Registry rs, int16_t imm12 = 0); ///< Load word at rs + imm12 into rd (with zero extension)
			INST put_ldu(Registry rd, Registry rs, int16_t imm12 = 0); ///< Load dword at rs + imm12 into rd (with zero extension)
			INST put_lq(Registry rd, Registry rs, int16_t imm12 = 0); ///< Load qword at rs + imm12 into rd
			INST put_sb(Registry rs1, Registry rs2, int16_t imm12 = 0); ///< Store byte from rs1 at rs2 + imm
			INST put_sw(Registry rs1, Registry rs2, int16_t imm12 = 0); ///< Store word from rs1 at rs2 + imm
			INST put_sd(Registry rs1, Registry rs2, int16_t imm12 = 0); ///< Store dword from rs1 at rs2 + imm
			INST put_sq(Registry rs1, Registry rs2, int16_t imm12 = 0); ///< Store qword from rs1 at rs2 + imm
			INST put_b(Condition cond, Registry rs1, Registry rs2, const Label& label); ///< Branch to label if condition is met between rs1 and rs2
			INST put_jal(const Label& label); ///< Jump and link
			INST put_jal(Registry rd, const Label& label); ///< Jump and link
			INST put_jalr(Registry rd, Registry rs, int16_t offset = 0); ///< Jump and link to register
			INST put_beq(Registry rs1, Registry rs2, const Label& label); ///< Branch if rs1 == rs2
			INST put_bne(Registry rs1, Registry rs2, const Label& label); ///< Branch if rs1 != rs2
			INST put_blt(Registry rs1, Registry rs2, const Label& label); ///< Branch if rs1 < rs2
			INST put_bge(Registry rs1, Registry rs2, const Label& label); ///< Branch if rs1 >= rs2
			INST put_bltu(Registry rs1, Registry rs2, const Label& label); ///< Branch if rs1 < rs2 (zero-extended)
			INST put_bgeu(Registry rs1, Registry rs2, const Label& label); ///< Branch if rs1 >= rs2 (zero-extended)
			INST put_bgt(Registry rs1, Registry rs2, const Label& label); ///< Branch if rs1 > rs2
			INST put_ble(Registry rs1, Registry rs2, const Label& label); ///< Branch if rs1 <= rs2
			INST put_bgtu(Registry rs1, Registry rs2, const Label& label); ///< Branch if rs1 > rs2 (zero-extended)
			INST put_bleu(Registry rs1, Registry rs2, const Label& label); ///< Branch if rs1 <= rs2 (zero-extended)
			INST put_lui(Registry rd, uint32_t imm20); ///< Load an upper constant to register, rd = imm << 12
			INST put_ecall(); ///< Environment Call
			INST put_ebreak(); ///< Environment Break
			INST put_auipc(Registry rd, const Label& label); ///< Add Upper Immediate to PC and store it in rd

			// "M" extension
			INST put_mul(Registry rd, Registry rs1, Registry rs2); ///< Store lower 64 bits of rs1 * rs2 into rd
			INST put_mulh(Registry rd, Registry rs1, Registry rs2); ///< Store upper 64 bits of rs1 * rs2 into rd
			INST put_mulhsu(Registry rd, Registry rs1, Registry rs2); ///< Store upper 64 bits of rs1 * rs2 into rd
			INST put_mulhu(Registry rd, Registry rs1, Registry rs2); ///< Store upper 64 bits of rs1 * rs2 into rd
			INST put_div(Registry rd, Registry rs1, Registry rs2); ///< Store rs1 / rs2 into rd
			INST put_divu(Registry rd, Registry rs1, Registry rs2); ///< Store rs1 / rs2 into rd
			INST put_rem(Registry rd, Registry rs1, Registry rs2); ///< Store reminder of rs1 / rs2 into rd
			INST put_remu(Registry rd, Registry rs1, Registry rs2); ///< Store reminder of rs1 / rs2 into rd
			INST put_muld(Registry rd, Registry rs1, Registry rs2); ///< Multiply dword rs1 and rs2, store result in rd
			INST put_divd(Registry rd, Registry rs1, Registry rs2); ///< Divide dword rs1 by rs2, store result in rd
			INST put_divud(Registry rd, Registry rs1, Registry rs2); ///< Unsigned divide dword rs1 by rs2, store result in rd
			INST put_remd(Registry rd, Registry rs1, Registry rs2); ///< Get reminder of dword rs1 and rs2 division, store result in rd
			INST put_remud(Registry rd, Registry rs1, Registry rs2); ///< Get reminder of dword rs1 and rs2 unsigned division, store result in rd

			// "A" extension
			INST put_lr(Registry rd, Registry rs, Size s = DWORD, Order order = Order::NONE); ///< Load reserved
			INST put_sc(Registry rd, Registry rt, Registry rs, Size s = DWORD, Order order = Order::NONE); ///< Store conditional, writes rs at address rt, status (0 on success, 1 otherwise) is written to rd
			INST put_amoswap(Registry old, Registry ptr, Registry val, Size s = DWORD, Order order = Order::NONE); ///< Atomic Swap
			INST put_amoadd(Registry old, Registry ptr, Registry val, Size s = DWORD, Order order = Order::NONE); ///< Atomic Add
			INST put_amoand(Registry old, Registry ptr, Registry val, Size s = DWORD, Order order = Order::NONE); ///< Atomic bitwise AND
			INST put_amoor(Registry old, Registry ptr, Registry val, Size s = DWORD, Order order = Order::NONE); ///< Atomic bitwise OR
			INST put_amoxor(Registry old, Registry ptr, Registry val, Size s = DWORD, Order order = Order::NONE); ///< Atomic bitwise XOR
			INST put_amomax(Registry old, Registry ptr, Registry val, Size s = DWORD, Order order = Order::NONE); ///< Atomic Maximum
			INST put_amomin(Registry old, Registry ptr, Registry val, Size s = DWORD, Order order = Order::NONE); ///< Atomic Minimum
			INST put_amomaxu(Registry old, Registry ptr, Registry val, Size s = DWORD, Order order = Order::NONE); ///< Atomic Maximum (unsigned)
			INST put_amominu(Registry old, Registry ptr, Registry val, Size s = DWORD, Order order = Order::NONE); ///< Atomic Minimum (unsigned)

			// Aliases
			INST put_mov(Registry rd, Registry rs); ///< Copy value from rs to rd
			INST put_mov(Registry rd, uint64_t imm); ///< Load immediate into rd
			INST put_mov(Registry rd, const Label& label); ///< Load address of label into rd
			INST put_nop(); ///< No Operation
			INST put_not(Registry rd, Registry rs); ///< Invert bits
			INST put_neg(Registry rd, Registry rs); ///< Negate two-complement number
			INST put_j(const Label& label); ///< Jump
			INST put_jr(Registry rs); ///< Jump register
			INST put_jlr(Registry rd, Registry rs); ///< Jump register with link
			INST put_ret(); ///< Return

	};

}
