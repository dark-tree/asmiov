#pragma once

#include <asmio/program/writer.hpp>
#include <asmio/riscv/argument/registry.hpp>
#include <asmio/riscv/argument/condition.hpp>

namespace asmio::riscv {

	class BufferWriter : public BasicBufferWriter {

		protected:

			void put_inst_r(uint8_t func7, Registry rs2, Registry rs1, uint8_t func3, Registry rd, uint8_t opc7);
			void put_inst_i(uint16_t imm12, Registry rs1, uint8_t func3, Registry rd, uint8_t opc7);
			void put_inst_s(uint16_t imm12, Registry rs2, Registry rs1, uint8_t func3, uint8_t opc7);
			void put_inst_b(uint16_t imm12, Registry rs2, Registry rs1, uint8_t func3, uint8_t opc7);
			void put_inst_u(uint32_t imm20, Registry rd, uint8_t opc7);
			void put_inst_j(uint32_t imm20, Registry rd, uint8_t opc7);

		public:

			BufferWriter(SegmentedBuffer& buffer);

			INST put_add(Registry rd, Registry rs, int16_t imm12); ///< Add signed 12 bit immediate to rs and save result to rd
			INST put_xor(Registry rd, Registry rs, int16_t imm12); ///< Bitwise XOR 12 bit immediate rs and save result to rd
			INST put_or(Registry rd, Registry rs, int16_t imm12); ///< Bitwise OR 12 bit immediate rs and save result to rd
			INST put_and(Registry rd, Registry rs, int16_t imm12); ///< Bitwise AND 12 bit immediate rs and save result to rd
			INST put_sll(Registry rd, Registry rs, int16_t imm12); ///< Shift register rs left by a 12 bit immediate and save result to rd
			INST put_srl(Registry rd, Registry rs, int16_t imm12); ///< Shift register rs right logically by a 12 bit immediate and save result to rd
			INST put_sra(Registry rd, Registry rs, int16_t imm12); ///< Shift register rs right arythetically by a 12 bit immediate and save result to rd
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

			INST put_lb(Registry rd, Registry rs, int16_t imm12); ///< Load byte at rs + imm12 into rd (with sign extension)
			INST put_lw(Registry rd, Registry rs, int16_t imm12); ///< Load word at rs + imm12 into rd (with sign extension)
			INST put_ld(Registry rd, Registry rs, int16_t imm12); ///< Load dword at rs + imm12 into rd (with sign extension)
			INST put_lbu(Registry rd, Registry rs, int16_t imm12); ///< Load byte at rs + imm12 into rd (with zero extension)
			INST put_lwu(Registry rd, Registry rs, int16_t imm12); ///< Load word at rs + imm12 into rd (with zero extension)
			INST put_ldu(Registry rd, Registry rs, int16_t imm12); ///< Load dword at rs + imm12 into rd (with zero extension)
			INST put_lq(Registry rd, Registry rs, int16_t imm12); ///< Load qword at rs + imm12 into rd

			INST put_sb(Registry rs1, Registry rs2, int16_t imm12); ///< Store byte from rs1 at rs2 + imm
			INST put_sw(Registry rs1, Registry rs2, int16_t imm12); ///< Store word from rs1 at rs2 + imm
			INST put_sd(Registry rs1, Registry rs2, int16_t imm12); ///< Store dword from rs1 at rs2 + imm
			INST put_sq(Registry rs1, Registry rs2, int16_t imm12); ///< Store qword from rs1 at rs2 + imm

			INST put_b(Condition cond, Registry rs1, Registry rs2, const Label& label); ///< Branch to label if condition is met between rs1 and rs2

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

			INST put_ecall(); ///< Environment Call
			INST put_ebreak(); ///< Environment Break

	};

}