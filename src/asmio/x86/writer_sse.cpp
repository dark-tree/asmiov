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

		if (loc.size != XMMWORD && loc.size != VOID) {
			throw std::runtime_error {"Invalid operand, expected xmmword register or memory"};
		}

		put_inst_std(opcode, loc, reg.pack(), XMMWORD, true);
	}

	void BufferWriter::put_inst_sse_sized(uint8_t opcode, Registry reg, const Location& loc, uint8_t size, bool prefix) {

		if (!reg.is(Registry::XMM)) {
			throw std::runtime_error {"Invalid operand, expected XMM register"};
		}

		if ((loc.is_simple() && loc.base.is(Registry::XMM)) || (loc.is_memory() && loc.size == size)) {
			if (prefix) {
				put_byte(0xF3);
			}

			put_inst_std(opcode, loc, reg.pack(), DWORD, true);
			return;
		}

		throw std::runtime_error {"Invalid operands"};
	}

	void BufferWriter::put_inst_mxcsr(const Location& loc, uint8_t opcode) {
		if (loc.is_memory() && loc.size == DWORD) {
			put_inst_std(0xAE, loc, RegInfo::raw(opcode), DWORD, true);
			return;
		}

		throw std::runtime_error {"Invalid operand, expected memory reference"};
	}

	void BufferWriter::put_inst_movxps(Location dst, Location src, uint8_t opcode) {
		if (dst.is_simple() && src.is_memory()) {
			if (src.size != QWORD) {
				throw std::runtime_error {"Invalid operand, expected QWORD memory reference"};
			}

			put_inst_std(opcode, src, dst.base.pack(), DWORD, true);
			return;
		}

		if (src.is_simple() && dst.is_memory()) {
			if (dst.size != QWORD) {
				throw std::runtime_error {"Invalid operand, expected QWORD memory reference"};
			}

			// set the direction flag in opcode
			put_inst_std(opcode | 1, dst, src.base.pack(), DWORD, true);
			return;
		}

		throw std::runtime_error {"Invalid operands"};
	}

	/// Move Aligned Packed f32 Values
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

	/// Move Packed f32 Values High to Low
	void BufferWriter::put_movhlps(Registry dst, Registry src) {
		put_inst_sse_2xmm(0x12, dst, src);
	}

	/// Move Packed f32 Values Low to High
	void BufferWriter::put_movlhps(Registry dst, Registry src) {
		put_inst_sse_2xmm(0x16, dst, src);
	}

	/// Move two packed f32 values from m64 to high quadword of dst
	void BufferWriter::put_movhps(Location dst, Location src) {
		put_inst_movxps(dst, src, 0x16);
	}

	/// Move two packed f32 values from m64 to low quadword of dst
	void BufferWriter::put_movlps(Location dst, Location src) {
		put_inst_movxps(dst, src, 0x12);
	}

	/// Extract Packed f32 Sign Mask
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

	/// Move or Merge Scalar f32 Value
	void BufferWriter::put_movss(Location dst, Location src) {

		if (dst.is_simple()) {
			put_inst_sse_sized(0x10, dst.base, src, DWORD);
			return;
		}

		if (src.is_simple()) {
			put_inst_sse_sized(0x11, src.base, dst, DWORD);
			return;
		}

		throw std::runtime_error {"Invalid operands"};
	}

	/// Move Unaligned Packed f32 Values
	void BufferWriter::put_movups(Location dst, Location src) {

		if (dst.is_simple()) {
			put_inst_sse(0x10, dst.base, src);
			return;
		}

		if (src.is_simple()) {
			put_inst_sse(0x11, src.base, dst);
			return;
		}

		throw std::runtime_error {"Invalid operands"};
	}

	/// Add Packed f32 values
	void BufferWriter::put_addps(Registry dst, Location src) {
		put_inst_sse(0x58, dst, src);
	}

	/// Add Scalar f32 Values
	void BufferWriter::put_addss(Registry dst, Location src) {
		put_inst_sse_sized(0x58, dst, src, DWORD);
	}

	/// Divide Packed f32 values
	void BufferWriter::put_divps(Registry dst, Location src) {
		put_inst_sse(0x5E, dst, src);
	}

	/// Divide Scalar f32 values
	void BufferWriter::put_divss(Registry dst, Location src) {
		put_inst_sse_sized(0x5E, dst, src, DWORD);
	}

	/// Compute Maximum of packed f32 values
	void BufferWriter::put_maxps(Registry dst, Location src) {
		put_inst_sse(0x5F, dst, src);
	}

	/// Compute Maximum of Scalar f32 values
	void BufferWriter::put_maxss(Registry dst, Location src) {
		put_inst_sse_sized(0x5F, dst, src, DWORD);
	}

	/// Compute Minimum of packed f32 values
	void BufferWriter::put_minps(Registry dst, Location src) {
		put_inst_sse(0x5D, dst, src);
	}

	/// Compute Minimum of Scalar f32 values
	void BufferWriter::put_minss(Registry dst, Location src) {
		put_inst_sse_sized(0x5D, dst, src, DWORD);
	}

	/// Multiply Packed f32 Values
	void BufferWriter::put_mulps(Registry dst, Location src) {
		put_inst_sse(0x59, dst, src);
	}

	/// Multiply Scalar f32 Values
	void BufferWriter::put_mulss(Registry dst, Location src) {
		put_inst_sse_sized(0x59, dst, src, DWORD);
	}

	/// Compute Reciprocals of Packed f32 values
	void BufferWriter::put_rcpps(Registry dst, Location src) {
		put_inst_sse(0x53, dst, src);
	}

	/// Compute Reciprocals of Scalar f32 values
	void BufferWriter::put_rcpss(Registry dst, Location src) {
		put_inst_sse_sized(0x53, dst, src, DWORD);
	}

	/// Compute Square Root Reciprocals of Packed f32 values
	void BufferWriter::put_rsqrtps(Registry dst, Location src) {
		put_inst_sse(0x52, dst, src);
	}

	/// Compute Square Root Reciprocals of Scalar f32 values
	void BufferWriter::put_rsqrtss(Registry dst, Location src) {
		put_inst_sse_sized(0x52, dst, src, DWORD);
	}

	/// Compute Square Roots of Packed f32 values
	void BufferWriter::put_sqrtps(Registry dst, Location src) {
		put_inst_sse(0x51, dst, src);
	}

	/// Compute Square Roots of Scalar f32 values
	void BufferWriter::put_sqrtss(Registry dst, Location src) {
		put_inst_sse_sized(0x51, dst, src, DWORD);
	}

	/// Subtract Packed f32 values
	void BufferWriter::put_subps(Registry dst, Location src) {
		put_inst_sse(0x5C, dst, src);
	}

	/// Subtract Scalar f32 values
	void BufferWriter::put_subss(Registry dst, Location src) {
		put_inst_sse_sized(0x5C, dst, src, DWORD);
	}

	/// Compare Packed f32 values
	void BufferWriter::put_cmpps(Registry dst, Location src, SimdCondition cond) {
		put_inst_sse(0xC2, dst, src);
		put_byte(static_cast<uint8_t>(cond));
	}

	/// Compare Scalar f32 values
	void BufferWriter::put_cmpss(Registry dst, Location src, SimdCondition cond) {
		put_inst_sse_sized(0xC2, dst, src, DWORD);
		put_byte(static_cast<uint8_t>(cond));
	}

	/// Compare Scalar Ordered f32 Values and Set EFLAGS
	void BufferWriter::put_comiss(Registry dst, Location src) {
		put_inst_sse_sized(0x2F, dst, src, DWORD, false);
	}

	/// Compare Scalar Unordered f32 Values and Set EFLAGS
	void BufferWriter::put_ucomiss(Registry dst, Location src) {
		put_inst_sse_sized(0x2E, dst, src, DWORD, false);
	}

	/// Bitwise logical AND NOT of packed dword values
	void BufferWriter::put_andnps(Registry dst, Location src) {
		put_inst_sse(0x55, dst, src);
	}

	/// Bitwise logical AND of packed dword values
	void BufferWriter::put_andps(Registry dst, Location src) {
		put_inst_sse(0x54, dst, src);
	}

	/// Bitwise logical OR of packed dword values
	void BufferWriter::put_orps(Registry dst, Location src) {
		put_inst_sse(0x56, dst, src);
	}

	/// Bitwise logical XOR of packed dword values
	void BufferWriter::put_xorps(Registry dst, Location src) {
		put_inst_sse(0x57, dst, src);
	}

	/// Packed Interleave Shuffle of Quadruplets of f32 Values
	void BufferWriter::put_shufps(Registry dst, Location src, uint8_t selector) {
		put_inst_sse(0xC6, dst, src);
		put_byte(selector);
	}

	/// Unpack and Interleave High Packed f32 Values
	void BufferWriter::put_unpckhps(Registry dst, Location src) {
		put_inst_sse(0x15, dst, src);
	}

	/// Unpack and Interleave Low Packed f32 Values
	void BufferWriter::put_unpcklps(Registry dst, Location src) {
		put_inst_sse(0x14, dst, src);
	}

	/// Convert Doubleword Integer to Scalar f32 Value
	void BufferWriter::put_cvtsi2ss(Registry dst, Location src) {
		put_byte(0xF3);
		put_inst_std(0x2A, src, dst.pack(), src.size, true);
	}

	/// Convert Scalar f32 Value to Doubleword Integer
	void BufferWriter::put_cvtss2si(Registry dst, Location src) {

		if (!dst.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid destination operand, expected general purpose register"};
		}

		if (dst.size != DWORD && dst.size != QWORD) {
			throw std::runtime_error {"Invalid destination operand, expected dword or qword register"};
		}

		if (src.is_simple() && !src.base.is(Registry::XMM)) {
			throw std::runtime_error {"Invalid source operand, expected XMM register"};
		}

		if (src.is_memory() && (src.size != VOID && src.size != DWORD)) {
			throw std::runtime_error {"Invalid source operand, dword memory reference"};
		}

		put_byte(0xF3);
		put_inst_std(0x2D, src, dst.pack(), dst.size, true);
	}

	/// Convert With Truncation Scalar f32 Value to Integer
	void BufferWriter::put_cvttss2si(Registry dst, Location src) {

		if (!dst.is(Registry::GENERAL)) {
			throw std::runtime_error {"Invalid destination operand, expected general purpose register"};
		}

		if (dst.size != DWORD && dst.size != QWORD) {
			throw std::runtime_error {"Invalid destination operand, expected dword or qword register"};
		}

		if (src.is_simple() && !src.base.is(Registry::XMM)) {
			throw std::runtime_error {"Invalid source operand, expected XMM register"};
		}

		if (src.is_memory() && (src.size != VOID && src.size != DWORD)) {
			throw std::runtime_error {"Invalid source operand, dword memory reference"};
		}

		put_byte(0xF3);
		put_inst_std(0x2C, src, dst.pack(), dst.size, true);
	}

	/// Load MXCSR Register from src
	void BufferWriter::put_ldmxcsr(Location src) {
		put_inst_mxcsr(src, 2);
	}

	/// Store MXCSR Register into dst
	void BufferWriter::put_stmxcsr(Location dst) {
		put_inst_mxcsr(dst, 3);
	}

	/// Store Packed f32 Values Using Non-Temporal Hint
	void BufferWriter::put_movntps(Location dst, Registry src) {
		if (dst.is_memory()) {
			put_inst_sse(0x2B, src, dst);
			return;
		}

		throw std::runtime_error {"Invalid destination operand, expected memory reference"};
	}

	/// Store Fence
	void BufferWriter::put_sfence() {
		put_byte(0x0F);
		put_byte(0xAE);
		put_byte(0xF8);
	}

}