#include "writer.hpp"

namespace asmio::x86 {

	void BufferWriter::put_inst_sse_2xmm(uint8_t opcode, Registry dst, Registry src) {
		if (dst.is(Registry::XMM) && src.is(Registry::XMM)) {
			put_inst_std(opcode, src, dst.pack(), XMMWORD, true);
			return;
		}

		throw std::runtime_error {"Invalid operands, expected two XMM registers"};
	}

	void BufferWriter::put_inst_sse(uint8_t opcode, Registry reg, const Location& loc) {

		if (!reg.is(Registry::XMM)) {
			throw std::runtime_error {"Invalid operand, expected XMM register"};
		}

		if (loc.size != XMMWORD) {
			throw std::runtime_error {"Invalid operand, expected xmmword register or memory"};
		}

		put_inst_std(opcode, loc, reg.pack(), XMMWORD, true);
	}

	void BufferWriter::put_inst_sse_sized(uint8_t opcode, Registry reg, const Location& loc, uint8_t size) {

		if (!reg.is(Registry::XMM)) {
			throw std::runtime_error {"Invalid operand, expected XMM register"};
		}

		if ((loc.is_simple() && loc.base.is(Registry::XMM)) || (loc.is_memory() && loc.size == size)) {
			put_byte(0xF3);
			put_inst_std(opcode, loc, reg.pack(), DWORD, true);
			return;
		}

		throw std::runtime_error {"Invalid operands"};
	}

	void BufferWriter::put_movaps(Location dst, Location src) {

		if (dst.is_simple()) {
			put_inst_sse(0x28, dst.base, src);
			return;
		}

		if (src.is_simple()) {
			put_inst_sse(0x29, src.base, dst);
			return;
		}

		throw std::runtime_error {"Invalid operands"};
	}

	void BufferWriter::put_movhlps(Registry dst, Registry src) {
		put_inst_sse_2xmm(0x12, dst, src);
	}

	void BufferWriter::put_movlhps(Registry dst, Registry src) {
		put_inst_sse_2xmm(0x16, dst, src);
	}

	void BufferWriter::put_movhps(Location dst, Location src) {

		if (dst.is_simple() && src.is_memory()) {
			if (src.size != QWORD) {
				throw std::runtime_error {"Invalid operand, expected QWORD memory reference"};
			}

			put_inst_std(0x16, src, dst.base.pack(), DWORD, true);
			return;
		}

		if (src.is_simple() && dst.is_memory()) {
			if (dst.size != QWORD) {
				throw std::runtime_error {"Invalid operand, expected QWORD memory reference"};
			}

			put_inst_std(0x17, dst, src.base.pack(), DWORD, true);
			return;
		}

		throw std::runtime_error {"Invalid operands"};
	}

	void BufferWriter::put_movlps(Location dst, Location src) {

		if (dst.is_simple() && src.is_memory()) {
			if (src.size != QWORD) {
				throw std::runtime_error {"Invalid operand, expected QWORD memory reference"};
			}

			put_inst_std(0x12, src, dst.base.pack(), DWORD, true);
			return;
		}

		if (src.is_simple() && dst.is_memory()) {
			if (dst.size != QWORD) {
				throw std::runtime_error {"Invalid operand, expected QWORD memory reference"};
			}

			put_inst_std(0x13, dst, src.base.pack(), DWORD, true);
			return;
		}

		throw std::runtime_error {"Invalid operands"};
	}

	void BufferWriter::put_movmskps(Registry dst, Registry src) {

		if (!dst.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid destination operand, expected general purpose register"};
		}

		if (dst.size != QWORD && dst.size != DWORD) {
			throw std::runtime_error {"Invalid destination operand, expected dword or qword register"};
		}

		if (!src.is(Registry::XMM)) {
			throw std::runtime_error {"Invalid source operand, expected XMM register"};
		}

		put_inst_std(0x50, src, dst.pack(), dst.size, true);
	}

	void BufferWriter::put_addps(Registry dst, Location src) {
		put_inst_sse(0x58, dst, src);
	}

	void BufferWriter::put_addss(Registry dst, Location src) {
		put_inst_sse_sized(0x58, dst, src, DWORD);
	}

	void BufferWriter::put_divps(Registry dst, Location src) {
		put_inst_sse(0x5E, dst, src);
	}

	void BufferWriter::put_divss(Registry dst, Location src) {
		put_inst_sse_sized(0x5E, dst, src, DWORD);
	}

	void BufferWriter::put_maxps(Registry dst, Location src) {
		put_inst_sse(0x5F, dst, src);
	}

	void BufferWriter::put_maxss(Registry dst, Location src) {
		put_inst_sse_sized(0x5F, dst, src, DWORD);
	}

	void BufferWriter::put_minps(Registry dst, Location src) {
		put_inst_sse(0x5D, dst, src);
	}

	void BufferWriter::put_minss(Registry dst, Location src) {
		put_inst_sse_sized(0x5D, dst, src, DWORD);
	}

	void BufferWriter::put_mulps(Registry dst, Location src) {
		put_inst_sse(0x59, dst, src);
	}

	void BufferWriter::put_mulss(Registry dst, Location src) {
		put_inst_sse_sized(0x59, dst, src, DWORD);
	}

	void BufferWriter::put_rcpps(Registry dst, Location src) {
		put_inst_sse(0x53, dst, src);
	}

	void BufferWriter::put_rcpss(Registry dst, Location src) {
		put_inst_sse_sized(0x53, dst, src, DWORD);
	}

	void BufferWriter::put_rsqrtps(Registry dst, Location src) {
		put_inst_sse(0x52, dst, src);
	}

	void BufferWriter::put_rsqrtss(Registry dst, Location src) {
		put_inst_sse_sized(0x52, dst, src, DWORD);
	}

	void BufferWriter::put_sqrtps(Registry dst, Location src) {
		put_inst_sse(0x51, dst, src);
	}

	void BufferWriter::put_sqrtss(Registry dst, Location src) {
		put_inst_sse_sized(0x51, dst, src, DWORD);
	}

	void BufferWriter::put_subps(Registry dst, Location src) {
		put_inst_sse(0x5C, dst, src);
	}

	void BufferWriter::put_subss(Registry dst, Location src) {
		put_inst_sse_sized(0x5C, dst, src, DWORD);
	}

	void BufferWriter::put_cmpps(Registry dst, Location src, SimdCondition cond) {
		put_inst_sse(0xC2, dst, src);
		put_byte(static_cast<uint8_t>(cond));
	}

	void BufferWriter::put_cmpss(Registry dst, Location src, SimdCondition cond) {
		put_inst_sse_sized(0xC2, dst, src, DWORD);
		put_byte(static_cast<uint8_t>(cond));
	}

	void BufferWriter::put_andnps(Registry dst, Location src) {
		put_inst_sse(0x55, dst, src);
	}

	void BufferWriter::put_andps(Registry dst, Location src) {
		put_inst_sse(0x54, dst, src);
	}

	void BufferWriter::put_orps(Registry dst, Location src) {
		put_inst_sse(0x56, dst, src);
	}

	void BufferWriter::put_xorps(Registry dst, Location src) {
		put_inst_sse(0x57, dst, src);
	}

	void BufferWriter::put_cvtsi2ss(Registry dst, Location src) {
		put_byte(0xF3);
		put_inst_std(0x2A, src, dst.pack(), src.size, true);
	}

}