
#include "../writer.hpp"

namespace asmio::arm {

	/*
	 * class BufferWriter
	 */

	void BufferWriter::put_b(Label label) {
		put_link(26, 0, label);
		put_dword(0b000101 << 26);
	}

	void BufferWriter::put_b(Condition condition, Label label) {
		put_link(19, 5, label);
		put_dword(0b01010100 << 24 | uint8_t(condition));
	}

	void BufferWriter::put_bl(Label label) {
		put_link(26, 0, label);
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

	void BufferWriter::put_cbnz(Registry src, Label label) {
		uint16_t sf = src.wide() ? 1 : 0;
		put_link(19, 5, label);
		put_dword(sf << 31 | 0b011010'1 << 24 | src.reg);
	}

	void BufferWriter::put_cbz(Registry src, Label label) {
		uint16_t sf = src.wide() ? 1 : 0;
		put_link(19, 5, label);
		put_dword(sf << 31 | 0b011010'0 << 24 | src.reg);
	}

}