#include "writer.hpp"

namespace asmio::riscv {
	/*
	 * class BufferWriter
	 */

	BufferWriter::BufferWriter(SegmentedBuffer& buffer)
		: BasicBufferWriter(buffer) {
	}

	void BufferWriter::put_inst_r(uint8_t func7, Registry rs2, Registry rs1, uint8_t func3, Registry rd, uint8_t opc7) {
		put_dword((func7 & 0b1111111) << 25 | rs2.reg << 20 | rs1.reg << 15 | (func3 & 0b111) << 12 | rd.reg << 7 | (opc7 & 0b1111111));
	}

	void BufferWriter::put_inst_i(uint16_t imm12, Registry rs1, uint8_t func3, Registry rd, uint8_t opc7) {
		put_dword((imm12 & 0xfff) << 20 | rs1.reg << 15 | (func3 & 0b111) << 12 | rd.reg << 7 | (opc7 & 0b1111111));
	}

	void BufferWriter::put_inst_s(uint16_t imm12, Registry rs2, Registry rs1, uint8_t func3, uint8_t opc7) {
		const uint32_t hi = (imm12 & 0b1111111'00000) >> 5;
		const uint32_t lo = (imm12 & 0b0000000'11111) >> 0;

		put_dword(hi << 25 | rs2.reg << 20 | rs1.reg << 15 | (func3 & 0b111) << 12 | lo << 7 | (opc7 & 0b1111111));
	}

	void BufferWriter::put_inst_b(uint16_t imm12, Registry rs2, Registry rs1, uint8_t func3, uint8_t opc7) {
		const uint32_t si = (imm12 & 0b1'0'00000'0000) >> 10;
		const uint32_t b7 = (imm12 & 0b0'1'00000'0000) >> 9;
		const uint32_t hi = (imm12 & 0b0'0'11111'0000) >> 4;
		const uint32_t lo = (imm12 & 0b0'0'00000'1111) >> 0;

		put_dword(si << 31 | hi << 25 | rs2.reg << 20 | rs1.reg << 15 | (func3 & 0b111) << 12 | lo << 8 | b7 << 7 | (opc7 & 0b1111111));

	}

	void BufferWriter::put_inst_u(uint32_t imm20, Registry rd, uint8_t opc7) {
		put_dword(imm20 << 20 | rd.reg << 7 | (opc7 & 0b1111111));
	}

	void BufferWriter::put_inst_j(uint32_t imm20, Registry rd, uint8_t opc7) {
		const uint32_t si = (imm20 & 0b1'00000000'0'0000000000) >> 19;
		const uint32_t hi = (imm20 & 0b0'11111111'0'0000000000) >> 11;
		const uint32_t b2 = (imm20 & 0b0'00000000'1'0000000000) >> 10;
		const uint32_t lo = (imm20 & 0b0'00000000'0'1111111111) >> 0;

		put_dword(si << 31 | lo << 21 | b2 << 20 | hi << 12 | rd.reg << 7 | (opc7 & 0b1111111));
	}

	void BufferWriter::put_add(Registry rd, Registry rs, int16_t imm12) {
		put_inst_i(imm12, rs, 0x0, rd, 0b0010011);
	}

	void BufferWriter::put_xor(Registry rd, Registry rs, int16_t imm12) {
		put_inst_i(imm12, rs, 0x4, rd, 0b0010011);
	}

	void BufferWriter::put_or(Registry rd, Registry rs, int16_t imm12) {
		put_inst_i(imm12, rs, 0x6, rd, 0b0010011);
	}

	void BufferWriter::put_and(Registry rd, Registry rs, int16_t imm12) {
		put_inst_i(imm12, rs, 0x7, rd, 0b0010011);
	}

	void BufferWriter::put_sll(Registry rd, Registry rs, int16_t imm12) {
		imm12 &= 0b0000000'11111;
		put_inst_i(imm12, rs, 0x1, rd, 0b0010011);
	}

	void BufferWriter::put_srl(Registry rd, Registry rs, int16_t imm12) {
		imm12 &= 0b0000000'11111;
		put_inst_i(imm12, rs, 0x5, rd, 0b0010011);

	}

	void BufferWriter::put_sra(Registry rd, Registry rs, int16_t imm12) {
		imm12 &= 0b0000000'11111;
		imm12 |= 0b0100000'00000;
		put_inst_i(imm12, rs, 0x5, rd, 0b0010011);

	}

	void BufferWriter::put_slt(Registry rd, Registry rs, int16_t imm12) {
		put_inst_i(imm12, rs, 0x2, rd, 0b0010011);
	}

	void BufferWriter::put_sltu(Registry rd, Registry rs, int16_t imm12) {
		put_inst_i(imm12, rs, 0x3, rd, 0b0010011);
	}

	void BufferWriter::put_add(Registry rd, Registry rs1, Registry rs2) {
		put_inst_r(0x00, rs2, rs1, 0x0, rd, 0b0110011);
	}

	void BufferWriter::put_sub(Registry rd, Registry rs1, Registry rs2) {
		put_inst_r(0x20, rs2, rs1, 0x0, rd, 0b0110011);
	}

	void BufferWriter::put_xor(Registry rd, Registry rs1, Registry rs2) {
		put_inst_r(0x00, rs2, rs1, 0x4, rd, 0b0110011);
	}

	void BufferWriter::put_or(Registry rd, Registry rs1, Registry rs2) {
		put_inst_r(0x00, rs2, rs1, 0x6, rd, 0b0110011);
	}

	void BufferWriter::put_and(Registry rd, Registry rs1, Registry rs2) {
		put_inst_r(0x00, rs2, rs1, 0x7, rd, 0b0110011);
	}

	void BufferWriter::put_sll(Registry rd, Registry rs1, Registry rs2) {
		put_inst_r(0x00, rs2, rs1, 0x1, rd, 0b0110011);
	}

	void BufferWriter::put_srl(Registry rd, Registry rs1, Registry rs2) {
		put_inst_r(0x00, rs2, rs1, 0x5, rd, 0b0110011);
	}

	void BufferWriter::put_sra(Registry rd, Registry rs1, Registry rs2) {
		put_inst_r(0x20, rs2, rs1, 0x5, rd, 0b0110011);
	}

	void BufferWriter::put_slt(Registry rd, Registry rs1, Registry rs2) {
		put_inst_r(0x00, rs2, rs1, 0x2, rd, 0b0110011);
	}

	void BufferWriter::put_sltu(Registry rd, Registry rs1, Registry rs2) {
		put_inst_r(0x00, rs2, rs1, 0x3, rd, 0b0110011);
	}

	void BufferWriter::put_lb(Registry rd, Registry rs, int16_t imm12) {
		put_inst_i(imm12, rs, 0x0, rd, 0b0000011);
	}

	void BufferWriter::put_lw(Registry rd, Registry rs, int16_t imm12) {
		put_inst_i(imm12, rs, 0x1, rd, 0b0000011);
	}

	void BufferWriter::put_ld(Registry rd, Registry rs, int16_t imm12) {
		put_inst_i(imm12, rs, 0x2, rd, 0b0000011);
	}

	void BufferWriter::put_lbu(Registry rd, Registry rs, int16_t imm12) {
		put_inst_i(imm12, rs, 0x4, rd, 0b0000011);
	}

	void BufferWriter::put_lwu(Registry rd, Registry rs, int16_t imm12) {
		put_inst_i(imm12, rs, 0x5, rd, 0b0000011);
	}

	void BufferWriter::put_ldu(Registry rd, Registry rs, int16_t imm12) {
		put_inst_i(imm12, rs, 0x6, rd, 0b0000011);
	}

	void BufferWriter::put_lq(Registry rd, Registry rs, int16_t imm12) {
		put_inst_i(imm12, rs, 0x3, rd, 0b0000011);
	}

	void BufferWriter::put_sb(Registry rs1, Registry rs2, int16_t imm12) {
		put_inst_s(imm12, rs1, rs2, 0x0, 0b0100011);
	}

	void BufferWriter::put_sw(Registry rs1, Registry rs2, int16_t imm12) {
		put_inst_s(imm12, rs1, rs2, 0x1, 0b0100011);
	}

	void BufferWriter::put_sd(Registry rs1, Registry rs2, int16_t imm12) {
		put_inst_s(imm12, rs1, rs2, 0x2, 0b0100011);
	}

	void BufferWriter::put_sq(Registry rs1, Registry rs2, int16_t imm12) {
		put_inst_s(imm12, rs1, rs2, 0x3, 0b0100011);
	}

	void BufferWriter::put_ecall() {
		put_inst_i(0, X0, 0x0, X0, 0b1110011);
	}

	void BufferWriter::put_ebreak() {
		put_inst_i(1, X0, 0x0, X0, 0b1110011);
	}

}