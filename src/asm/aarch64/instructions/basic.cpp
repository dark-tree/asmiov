#include "../writer.hpp"

namespace asmio::arm {

	/*
	 * class BufferWriter
	 */

	INST BufferWriter::put_nop() {
		put_dword(0b1101010100'0'00'011'0010'0000'000'11111);
	}

	INST BufferWriter::put_ret() {
		put_ret(X(30));
	}

	INST BufferWriter::put_ret(Registry registry) {

		if (!registry.wide()) {
			throw std::runtime_error {"Invalid operand, non-qword register can't be used here"};
		}

		if (registry.is(Registry::GENERAL)) {

		}

		put_dword(0b1101011001011111000000'00000'00000 | registry.reg << 5);
	}

}