#include "writer.hpp"

namespace asmio::riscv {
	/*
	 * class BufferWriter
	 */

	BufferWriter::BufferWriter(SegmentedBuffer& buffer)
		: BasicBufferWriter(buffer) {
	}

	void BufferWriter::put_inst_r(uint8_t func7, Registry rs2, Registry rs1, uint8_t func3, Registry rd, uint8_t opc7) {
		put_dword((func7 & 0b1111111) << 25 | rs2.reg << 20 | rs1.reg << 15 | (func3 & 0b111) << 12 | rd.reg << 7 | opc7 & 0b1111111);
	}

	void BufferWriter::put_inst_i(uint16_t imm12, Registry rs1, uint8_t func3, Registry rd, uint8_t opc7) {
		put_dword((imm12 & 0xfff) << 20 | rs1.reg << 15 | (func3 & 0b111) << 12 | rd.reg << 7 | opc7 & 0b1111111);
	}

	void BufferWriter::put_inst_s(uint16_t imm12, Registry rs2, Registry rs1, uint8_t func3, uint8_t opc7) {
		const uint32_t hi = (imm12 & 0b1111111'00000) >> 5;
		const uint32_t lo = (imm12 & 0b0000000'11111) >> 0;

		put_dword(hi << 25 | rs2.reg << 20 | rs1.reg << 15 | (func3 & 0b111) << 12 | lo << 7 | opc7 & 0b1111111);
	}

	void BufferWriter::put_inst_b(uint16_t imm12, Registry rs2, Registry rs1, uint8_t func3, uint8_t opc7) {
		const uint32_t si = (imm12 & 0b1'0'00000'0000) >> 10;
		const uint32_t b7 = (imm12 & 0b0'1'00000'0000) >> 9;
		const uint32_t hi = (imm12 & 0b0'0'11111'0000) >> 4;
		const uint32_t lo = (imm12 & 0b0'0'00000'1111) >> 0;

		put_dword(si << 31 | hi << 25 | rs2.reg << 20 | rs1.reg << 15 | (func3 & 0b111) << 12 | lo << 8 | b7 << 7 | opc7 & 0b1111111);

	}

	void BufferWriter::put_inst_u(uint32_t imm20, Registry rd, uint8_t opc7) {
		put_dword(imm20 << 20 | rd.reg << 7 | opc7 & 0b1111111);
	}

	void BufferWriter::put_inst_j(uint32_t imm20, Registry rd, uint8_t opc7) {
		const uint32_t si = (imm20 & 0b1'00000000'0'0000000000) >> 19;
		const uint32_t hi = (imm20 & 0b0'11111111'0'0000000000) >> 11;
		const uint32_t b2 = (imm20 & 0b0'00000000'1'0000000000) >> 10;
		const uint32_t lo = (imm20 & 0b0'00000000'0'1111111111) >> 0;

		put_dword(si << 31 | lo << 21 | b2 << 20 | hi << 12 | rd.reg << 7 | opc7 & 0b1111111);
	}

	void BufferWriter::put_add(Registry rd, Registry rs, int16_t imm12) {
		put_inst_i(imm12, rs, 0b000, rd, 0b0010011);
	}

}