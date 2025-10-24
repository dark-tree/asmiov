
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

}