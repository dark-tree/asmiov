
#include "../writer.hpp"

namespace asmio::arm {

	/*
	 * class BufferWriter
	 */

	void BufferWriter::put_b(const Label& label) {
		buffer.add_linkage(label, 0, link_26_0_aligned);
		put_dword(0b000101 << 26);
	}

	void BufferWriter::put_b(Condition condition, const Label& label) {
		buffer.add_linkage(label, 0, link_19_5_aligned);
		put_dword(0b01010100 << 24 | uint8_t(condition));
	}

	void BufferWriter::put_bl(const Label& label) {
		buffer.add_linkage(label, 0, link_26_0_aligned);
		put_dword(0b100101 << 26);
	}

	void BufferWriter::put_blr(Registry destination) {
		//                  Z   op            A M
		put_dword(0b1101011'0'0'01'11111'0000'0'0 << 10 | destination.reg << 5);
	}

	void BufferWriter::put_br(Registry destination) {
		//                  Z   op            A M
		put_dword(0b1101011'0'0'00'11111'0000'0'0 << 10 | destination.reg << 5);
	}

	void BufferWriter::put_cbnz(Registry src, const Label& label) {
		uint16_t sf = src.wide() ? 1 : 0;
		buffer.add_linkage(label, 0, link_19_5_aligned);
		put_dword(sf << 31 | 0b011010'1 << 24 | src.reg);
	}

	void BufferWriter::put_cbz(Registry src, const Label& label) {
		uint16_t sf = src.wide() ? 1 : 0;
		buffer.add_linkage(label, 0, link_19_5_aligned);
		put_dword(sf << 31 | 0b011010'0 << 24 | src.reg);
	}

	void BufferWriter::put_tbz(Registry test, uint16_t bit6, const Label& label) {
		uint32_t sf = (bit6 & 0b1'00000) >> 5;

		if (sf && !test.wide()) {
			throw std::runtime_error {"Invalid operands, expected qword register in this context"};
		}

		buffer.add_linkage(label, 0, link_14_5_aligned);
		put_dword(sf << 31 | 0b011011'0 << 24 | (0b11111 & bit6) << 19 | test.reg);
	}

	void BufferWriter::put_tbnz(Registry test, uint16_t bit6, const Label& label) {
		uint32_t sf = (bit6 & 0b1'00000) >> 5;

		if (sf && !test.wide()) {
			throw std::runtime_error {"Invalid operands, expected qword register in this context"};
		}

		buffer.add_linkage(label, 0, link_14_5_aligned);
		put_dword(sf << 31 | 0b011011'1 << 24 | (0b11111 & bit6) << 19 | test.reg);
	}

}