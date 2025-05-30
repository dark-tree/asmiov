#include "asm/x86/writer.hpp"

namespace asmio::x86 {

	/// No Operation
	void BufferWriter::put_fnop() {
		put_inst_fpu(0xD9, 0xD0);
	}

	/// Initialize FPU
	void BufferWriter::put_finit() {
		put_wait();
		put_fninit();
	}

	/// Initialize FPU (without checking for pending unmasked exceptions)
	void BufferWriter::put_fninit() {
		put_byte(0xDB);
		put_byte(0xE3);
	}

	/// Clear Exceptions
	void BufferWriter::put_fclex() {
		put_wait();
		put_fnclex();
	}

	/// Clear Exceptions (without checking for pending unmasked exceptions)
	void BufferWriter::put_fnclex() {
		put_inst_fpu(0xDB, 0xE2);
	}

	/// Store FPU State Word
	void BufferWriter::put_fstsw(Location dst) {
		put_wait();
		put_fnstsw(dst);
	}

	/// Store FPU State Word (without checking for pending unmasked exceptions)
	void BufferWriter::put_fnstsw(Location dst) {

		if (dst.is_simple() && dst.base.is(AX)) {
			put_inst_fpu(0xDF, 0xE0);
			return;
		}

		if (dst.is_memory() && dst.size == WORD) {
			put_inst_std_ri(0xDD, dst, 7);
			return;
		}

		if (dst.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected word"};
		}

		throw std::runtime_error{"Invalid operand"};

	}

	/// Store FPU Control Word
	void BufferWriter::put_fstcw(Location dst) {
		put_wait();
		put_fnstcw(dst);
	}

	/// Store FPU Control Word (without checking for pending unmasked exceptions)
	void BufferWriter::put_fnstcw(Location dst) {

		if (dst.is_memory() && dst.size == WORD) {
			put_inst_std_ri(0xD9, dst, 7);
			return;
		}

		if (dst.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected word"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Load +1.0 Constant onto the stack
	void BufferWriter::put_fld1() {
		put_inst_fpu(0xD9, 0xE8);
	}

	/// Load +0.0 Constant onto the stack
	void BufferWriter::put_fld0() {
		put_inst_fpu(0xD9, 0xEE);
	}

	/// Load PI Constant onto the stack
	void BufferWriter::put_fldpi() {
		put_inst_fpu(0xD9, 0xEB);
	}

	/// Load Log{2}(10) Constant onto the stack
	void BufferWriter::put_fldl2t() {
		put_inst_fpu(0xD9, 0xE9);
	}

	/// Load Log{2}(e) Constant onto the stack
	void BufferWriter::put_fldl2e() {
		put_inst_fpu(0xD9, 0xEA);
	}

	/// Load Log{10}(2) Constant onto the stack
	void BufferWriter::put_fldlt2() {
		put_inst_fpu(0xD9, 0xEC);
	}

	/// Load Log{e}(2) Constant onto the stack
	void BufferWriter::put_fldle2() {
		put_inst_fpu(0xD9, 0xED);
	}

	/// Load x87 FPU Control Word
	void BufferWriter::put_fldcw(Location src) {

		// fldcw src:m2byte
		if (src.is_memory() && src.size == WORD) {
			put_inst_std_ri(0xD9, src, 5);
			return;
		}

		if (src.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected word"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Compute 2^x - 1
	void BufferWriter::put_f2xm1() {
		put_inst_fpu(0xD9, 0xF0);
	}

	/// Absolute Value
	void BufferWriter::put_fabs() {
		put_inst_fpu(0xD9, 0xE1);
	}

	/// Change Sign
	void BufferWriter::put_fchs() {
		put_inst_fpu(0xD9, 0xE0);
	}

	/// Compute Cosine
	void BufferWriter::put_fcos() {
		put_inst_fpu(0xD9, 0xFF);
	}

	/// Compute Sine
	void BufferWriter::put_fsin() {
		put_inst_fpu(0xD9, 0xFE);
	}

	/// Compute Sine and Cosine, sets ST(0) to sin(ST(0)), and pushes cos(ST(0)) onto the stack
	void BufferWriter::put_fsincos() {
		put_inst_fpu(0xD9, 0xFB);
	}

	/// Decrement Stack Pointer
	void BufferWriter::put_fdecstp() {
		put_inst_fpu(0xD9, 0xF6);
	}

	/// Increment Stack Pointer
	void BufferWriter::put_fincstp() {
		put_inst_fpu(0xD9, 0xF7);
	}

	/// Partial Arctangent, sets ST(1) to arctan(ST(1) / ST(0)), and pops the stack
	void BufferWriter::put_fpatan() {
		put_inst_fpu(0xD9, 0xF3);
	}

	/// Partial Remainder, sets ST(0) to ST(0) % ST(1)
	void BufferWriter::put_fprem() {
		put_inst_fpu(0xD9, 0xF8);
	}

	/// Partial IEEE Remainder, sets ST(0) to ST(0) % ST(1)
	void BufferWriter::put_fprem1() {
		put_inst_fpu(0xD9, 0xF5);
	}

	/// Partial Tangent, sets ST(0) to tan(ST(0)), and pushes 1 onto the stack
	void BufferWriter::put_fptan() {
		put_inst_fpu(0xD9, 0xF2);
	}

	/// Round to Integer, Rounds ST(0) to an integer
	void BufferWriter::put_frndint() {
		put_inst_fpu(0xD9, 0xFC);
	}

	/// Scale, ST(0) by ST(1), Sets ST(0) to ST(0)*2^floor(ST(1))
	void BufferWriter::put_fscale() {
		put_inst_fpu(0xD9, 0xFD);
	}

	/// Square Root, sets ST(0) to sqrt(ST(0))
	void BufferWriter::put_fsqrt() {
		put_inst_fpu(0xD9, 0xFA);
	}

	/// Load Floating-Point Value
	void BufferWriter::put_fld(Location src) {

		// fld src:m32fp
		if (src.is_memory() && src.size == DWORD) {
			put_inst_std_ri(0xD9, src, 0);
			return;
		}

		// fld src:m64fp
		if (src.is_memory() && src.size == QWORD) {
			put_inst_std_ri(0xDD, src, 0);
			return;
		}

		// fld src:m80fp
		if (src.is_memory() && src.size == TWORD) {
			put_inst_std_ri(0xDB, src, 5);
			return;
		}

		// fld src:st(i)
		if (src.is_floating()) {
			put_inst_fpu(0xD9, 0xC0, src.offset);
			return;
		}

		if (src.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected dword, qword or tword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Load Integer Value
	void BufferWriter::put_fild(Location src) {

		// fild src:m16int
		if (src.is_memory() && src.size == WORD) {
			put_inst_std_ri(0xDF, src, 0);
			return;
		}

		// fild src:m32int
		if (src.is_memory() && src.size == DWORD) {
			put_inst_std_ri(0xDB, src, 0);
			return;
		}

		// fild src:m64int
		if (src.is_memory() && src.size == QWORD) {
			put_inst_std_ri(0xDF, src, 5);
			return;
		}

		if (src.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected word, dword or qword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Store Floating-Point Value
	void BufferWriter::put_fst(Location dst) {

		// fst dst:m32fp
		if (dst.is_memory() && dst.size == DWORD) {
			put_inst_std_ri(0xD9, dst, 2);
			return;
		}

		// fst dst:m64fp
		if (dst.is_memory() && dst.size == QWORD) {
			put_inst_std_ri(0xDD, dst, 2);
			return;
		}

		// fst src:st(i)
		if (dst.is_floating()) {
			put_inst_fpu(0xDD, 0xD0, dst.offset);
			return;
		}

		if (dst.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected dword or qword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Store Floating-Point Value and Pop
	void BufferWriter::put_fstp(Location dst) {

		// fstp dst:m32fp
		if (dst.is_memory() && dst.size == DWORD) {
			put_inst_std_ri(0xD9, dst, 3);
			return;
		}

		// fstp dst:m64fp
		if (dst.is_memory() && dst.size == QWORD) {
			put_inst_std_ri(0xDD, dst, 3);
			return;
		}

		// fstp dst:m80fp
		if (dst.is_memory() && dst.size == TWORD) {
			put_inst_std_ri(0xDB, dst, 7);
			return;
		}

		// fstp src:st(i)
		if (dst.is_floating()) {
			put_inst_fpu(0xDD, 0xD8, dst.offset);
			return;
		}

		if (dst.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected dword, qword or tword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Store Floating-Point Value As Integer
	void BufferWriter::put_fist(Location dst) {

		// fist src:m16int
		if (dst.is_memory() && dst.size == WORD) {
			put_inst_std_ri(0xDF, dst, 2);
			return;
		}

		// fist src:m32int
		if (dst.is_memory() && dst.size == DWORD) {
			put_inst_std_ri(0xDB, dst, 2);
			return;
		}

		if (dst.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected word or dword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Store Floating-Point Value As Integer And Pop
	void BufferWriter::put_fistp(Location dst) {

		// fist src:m16int
		if (dst.is_memory() && dst.size == WORD) {
			put_inst_std_ri(0xDF, dst, 3);
			return;
		}

		// fist src:m32int
		if (dst.is_memory() && dst.size == DWORD) {
			put_inst_std_ri(0xDB, dst, 3);
			return;
		}

		// fist src:m64int
		if (dst.is_memory() && dst.size == QWORD) {
			put_inst_std_ri(0xDF, dst, 7);
			return;
		}

		if (dst.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected word, dword or qword"};
		}

		throw std::runtime_error{"Invalid operand"};

	}

	/// Store Floating-Point Value As Integer With Truncation And Pop
	void BufferWriter::put_fisttp(Location dst) {

		// fist src:m16int
		if (dst.is_memory() && dst.size == WORD) {
			put_inst_std_ri(0xDF, dst, 1);
			return;
		}

		// fist src:m32int
		if (dst.is_memory() && dst.size == DWORD) {
			put_inst_std_ri(0xDB, dst, 1);
			return;
		}

		// fist src:m64int
		if (dst.is_memory() && dst.size == QWORD) {
			put_inst_std_ri(0xDD, dst, 1);
			return;
		}

		if (dst.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected word, dword or qword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Free Floating-Point Register
	void BufferWriter::put_ffree(Location src) {

		if (src.is_floating()) {
			put_inst_fpu(0xDD, 0xC0, src.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Move SRC, if below, into ST+0
	void BufferWriter::put_fcmovb(Location src) {

		if (src.is_floating()) {
			put_inst_fpu(0xDA, 0xC0, src.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Move SRC, if equal, into ST+0
	void BufferWriter::put_fcmove(Location src) {

		if (src.is_floating()) {
			put_inst_fpu(0xDA, 0xC8, src.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Move SRC, if below or equal, into ST+0
	void BufferWriter::put_fcmovbe(Location src) {

		if (src.is_floating()) {
			put_inst_fpu(0xDA, 0xD0, src.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Move SRC, if unordered with, into ST+0
	void BufferWriter::put_fcmovu(Location src) {

		if (src.is_floating()) {
			put_inst_fpu(0xDA, 0xD8, src.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Move SRC, if not below, into ST+0
	void BufferWriter::put_fcmovnb(Location src) {

		if (src.is_floating()) {
			put_inst_fpu(0xDB, 0xC0, src.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Move SRC, if not equal, into ST+0
	void BufferWriter::put_fcmovne(Location src) {

		if (src.is_floating()) {
			put_inst_fpu(0xDB, 0xC8, src.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Move SRC, if not below or equal, into ST+0
	void BufferWriter::put_fcmovnbe(Location src) {

		if (src.is_floating()) {
			put_inst_fpu(0xDB, 0xD0, src.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Move SRC, if not unordered with, into ST+0
	void BufferWriter::put_fcmovnu(Location src) {

		if (src.is_floating()) {
			put_inst_fpu(0xDB, 0xD8, src.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Compare ST+0 with SRC
	void BufferWriter::put_fcom(Location src) {

		// fcom src:m32fp
		if (src.is_memory() && src.size == DWORD) {
			put_inst_std_ri(0xD8, src, 2);
			return;
		}

		// fcom src:m64fp
		if (src.is_memory() && src.size == QWORD) {
			put_inst_std_ri(0xDC, src, 2);
			return;
		}

		// fcom st(0), src:st(i)
		if (src.is_floating()) {
			put_inst_fpu(0xD8, 0xD0, src.offset);
			return;
		}

		if (src.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected dword or qword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Compare ST+0 with SRC And Pop
	void BufferWriter::put_fcomp(Location src) {

		// fcomp src:m32fp
		if (src.is_memory() && src.size == DWORD) {
			put_inst_std_ri(0xD8, src, 3);
			return;
		}

		// fcomp src:m64fp
		if (src.is_memory() && src.size == QWORD) {
			put_inst_std_ri(0xDC, src, 3);
			return;
		}

		// fcomp st(0), src:st(i)
		if (src.is_floating()) {
			put_inst_fpu(0xD8, 0xD8, src.offset);
			return;
		}

		if (src.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected dword or qword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Compare ST+0 with ST+1 And Pop Both
	void BufferWriter::put_fcompp() {
		put_inst_fpu(0xDE, 0xD9);
	}

	/// Compare ST+0 with Integer SRC
	void BufferWriter::put_ficom(Location src) {

		// ficom src:m16int
		if (src.is_memory() && src.size == WORD) {
			put_inst_std_ri(0xDE, src, 2);
			return;
		}

		// ficom src:m32int
		if (src.is_memory() && src.size == DWORD) {
			put_inst_std_ri(0xDA, src, 2);
			return;
		}

		if (src.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected word or dword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Compare ST+0 with Integer SRC, And Pop
	void BufferWriter::put_ficomp(Location src) {

		// ficom src:m16int
		if (src.is_memory() && src.size == WORD) {
			put_inst_std_ri(0xDE, src, 3);
			return;
		}

		// ficom src:m32int
		if (src.is_memory() && src.size == DWORD) {
			put_inst_std_ri(0xDA, src, 3);
			return;
		}

		if (src.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected word or dword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Compare ST+0 with SRC and set EFLAGS
	void BufferWriter::put_fcomi(Location src) {

		// fcomi st(0), src:st(i)
		if (src.is_floating()) {
			put_inst_fpu(0xDB, 0xF0, src.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Compare ST+0 with SRC, Pop, and set EFLAGS
	void BufferWriter::put_fcomip(Location src) {

		// fcomip st(0), src:st(i)
		if (src.is_floating()) {
			put_inst_fpu(0xDF, 0xF0, src.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Compare, and check for ordered values, ST+0 with SRC and set EFLAGS
	void BufferWriter::put_fucomi(Location src) {

		// fcomi st(0), src:st(i)
		if (src.is_floating()) {
			put_inst_fpu(0xDB, 0xF8, src.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Compare, and check for ordered values, ST+0 with SRC, Pop, and set EFLAGS
	void BufferWriter::put_fucomip(Location src) {

		// fcomip st(0), src:st(i)
		if (src.is_floating()) {
			put_inst_fpu(0xDF, 0xF8, src.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Multiply By Memory Float
	void BufferWriter::put_fmul(Location src) {

		// fmul src:m32fp
		if (src.is_memory() && src.size == DWORD) {
			put_inst_std_ri(0xD8, src, 1);
			return;
		}

		// fmul src:m64fp
		if (src.is_memory() && src.size == QWORD) {
			put_inst_std_ri(0xDC, src, 1);
			return;
		}

		if (src.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected dword or qword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Multiply By Memory Integer
	void BufferWriter::put_fimul(Location src) {

		// fmul src:m32int
		if (src.is_memory() && src.size == DWORD) {
			put_inst_std_ri(0xDA, src, 1);
			return;
		}

		// fmul src:m16int
		if (src.is_memory() && src.size == WORD) {
			put_inst_std_ri(0xDE, src, 1);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Multiply
	void BufferWriter::put_fmul(Location dst, Location src) {

		// fmul dst:st(0), src:st(i)
		if (dst.is_st0() && src.is_floating()) {
			put_inst_fpu(0xD8, 0xC8, src.offset);
			return;
		}

		// fmul dst:st(i), src:st(0)
		if (dst.is_floating() && src.is_st0()) {
			put_inst_fpu(0xDC, 0xC8, dst.offset);
			return;
		}

		throw std::runtime_error {"Invalid operands"};

	}

	/// Multiply And Pop
	void BufferWriter::put_fmulp(Location dst) {

		// fmulp dst:st(i), st(0)
		if (dst.is_floating()) {
			put_inst_fpu(0xDE, 0xC8, dst.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Add Memory Float
	void BufferWriter::put_fadd(Location src) {

		// fadd src:m32fp
		if (src.is_memory() && src.size == DWORD) {
			put_inst_std_ri(0xD8, src, 0);
			return;
		}

		// fadd src:m64fp
		if (src.is_memory() && src.size == QWORD) {
			put_inst_std_ri(0xDC, src, 0);
			return;
		}

		if (src.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected dword or qword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Add Memory Integer
	void BufferWriter::put_fiadd(Location src) {

		// fadd src:m32int
		if (src.is_memory() && src.size == DWORD) {
			put_inst_std_ri(0xDA, src, 0);
			return;
		}

		// fadd src:m16int
		if (src.is_memory() && src.size == WORD) {
			put_inst_std_ri(0xDE, src, 0);
			return;
		}

		if (src.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected word or dword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Add
	void BufferWriter::put_fadd(Location dst, Location src) {

		// fadd dst:st(0), src:st(i)
		if (dst.is_st0() && src.is_floating()) {
			put_inst_fpu(0xD8, 0xC0, src.offset);
			return;
		}

		// fadd dst:st(i), src:st(0)
		if (dst.is_floating() && src.is_st0()) {
			put_inst_fpu(0xDC, 0xC0, dst.offset);
			return;
		}

		throw std::runtime_error {"Invalid operands"};

	}

	/// Add And Pop
	void BufferWriter::put_faddp(Location dst) {

		// faddp dst:st(i), st(0)
		if (dst.is_floating()) {
			put_inst_fpu(0xDE, 0xC0, dst.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Divide By Memory Float
	void BufferWriter::put_fdiv(Location src) {

		// fdiv src:m32fp
		if (src.is_memory() && src.size == DWORD) {
			put_inst_std_ri(0xD8, src, 6);
			return;
		}

		// fdiv src:m64fp
		if (src.is_memory() && src.size == QWORD) {
			put_inst_std_ri(0xDC, src, 6);
			return;
		}

		if (src.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected dword or qword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Divide By Memory Integer
	void BufferWriter::put_fidiv(Location src) {

		// fidiv src:m32int
		if (src.is_memory() && src.size == DWORD) {
			put_inst_std_ri(0xDA, src, 6);
			return;
		}

		// fidiv src:m16int
		if (src.is_memory() && src.size == WORD) {
			put_inst_std_ri(0xDE, src, 6);
			return;
		}

		if (src.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected word or dword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Divide
	void BufferWriter::put_fdiv(Location dst, Location src) {

		// fdiv dst:st(0), src:st(i)
		if (dst.is_st0() && src.is_floating()) {
			put_inst_fpu(0xD8, 0xF0, src.offset);
			return;
		}

		// fdiv dst:st(i), src:st(0)
		if (dst.is_floating() && src.is_st0()) {
			put_inst_fpu(0xDC, 0xF8, dst.offset);
			return;
		}

		throw std::runtime_error {"Invalid operands"};

	}

	/// Divide And Pop
	void BufferWriter::put_fdivp(Location dst) {

		// fdivp dst:st(i), st(0)
		if (dst.is_floating()) {
			put_inst_fpu(0xDE, 0xF8, dst.offset);
			return;
		}

		throw std::runtime_error{"Invalid operand"};

	}

	/// Divide Memory Float
	void BufferWriter::put_fdivr(Location src) {

		// fdivr src:m32fp
		if (src.is_memory() && src.size == DWORD) {
			put_inst_std_ri(0xD8, src, 7);
			return;
		}

		// fdivr src:m64fp
		if (src.is_memory() && src.size == QWORD) {
			put_inst_std_ri(0xDC, src, 7);
			return;
		}

		if (src.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected dword or qword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Divide Memory Integer
	void BufferWriter::put_fidivr(Location src) {

		// fidivr src:m32int
		if (src.is_memory() && src.size == DWORD) {
			put_inst_std_ri(0xDA, src, 7);
			return;
		}

		// fidivr src:m16int
		if (src.is_memory() && src.size == WORD) {
			put_inst_std_ri(0xDE, src, 7);
			return;
		}

		if (src.is_memory()) {
			throw std::runtime_error {"Invalid operand size, expected word or dword"};
		}

		throw std::runtime_error {"Invalid operand"};

	}

	/// Reverse Divide
	void BufferWriter::put_fdivr(Location dst, Location src) {

		// fdivr dst:st(0), src:st(i)
		if (dst.is_st0() && src.is_floating()) {
			put_inst_fpu(0xD8, 0xF8, src.offset);
			return;
		}

		// fdivr dst:st(i), src:st(0)
		if (dst.is_floating() && src.is_st0()) {
			put_inst_fpu(0xDC, 0xF0, dst.offset);
			return;
		}

		throw std::runtime_error {"Invalid operands"};

	}

	/// Reverse Divide And Pop
	void BufferWriter::put_fdivrp(Location dst) {

		// fdivrp dst:st(i), st(0)
		if (dst.is_floating()) {
			put_inst_fpu(0xDE, 0xF0, dst.offset);
			return;
		}

		throw std::runtime_error {"Invalid operand"};

	}

}