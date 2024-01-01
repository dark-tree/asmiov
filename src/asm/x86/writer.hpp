#pragma once

#include "external.hpp"
#include "address.hpp"
#include "label.hpp"
#include "buffer.hpp"
#include "file/elf/buffer.hpp"

namespace asmio::x86 {

	using namespace elf;

	class BufferWriter {

		private:

			std::unordered_map<Label, size_t, Label::HashFunction> labels;
			std::vector<uint8_t> buffer;
			std::vector<LabelCommand> commands;

			uint8_t pack_opcode_dw(uint8_t opcode, bool d, bool w);
			void put_inst_mod_reg_rm(uint8_t mod, uint8_t reg, uint8_t r_m);
			void put_inst_sib(uint8_t ss, uint8_t index, uint8_t base);
			void put_inst_imm(uint32_t immediate, uint8_t width);
			void put_inst_sib(Location ref);
			void put_inst_label_imm(Location imm, uint8_t size);
			void put_inst_std(uint8_t opcode, Location dst, uint8_t reg, bool longer = false);
			void put_inst_std(uint8_t opcode, Location dst, uint8_t reg, bool direction, bool wide, bool longer = false);
			void put_inst_fpu(uint8_t opcode, uint8_t base, uint8_t sti = 0);

			/**
			 * Used for constructing the MOV instruction
			 */
			void put_inst_mov(Location dst, Location src, bool direction);

			/**
			 * Used for constructing the MOVSX and MOVZX instructions
			 */
			void put_inst_movx(uint8_t opcode, Location dst, Location src);

			/**
			 * Used to for constructing the shift instructions
			 */
			void put_inst_shift(Location dst, Location src, uint8_t inst);

			/**
			 * Used for constructing a range of two argument instructions
			 */
			void put_inst_tuple(Location dst, Location src, uint8_t opcode_rmr, uint8_t opcode_reg);

			/**
			 * Used for constructing the Bit Test family of instructions
			 */
			void put_inst_btx(Location dst, Location src, uint8_t opcode, uint8_t inst);

			/**
			 * Used for constructing the conditional jump family of instructions
			 */
			void put_inst_jx(Label label, uint8_t sopcode, uint8_t lopcode);

			/**
			 * Used for constructing the 'set byte' family of instructions
			 */
			void put_inst_setx(Location dst, uint8_t lopcode);

			void put_inst_16bit_operand_mark();
			void put_inst_16bit_address_mark();
			void put_label(Label label, uint8_t size);
			bool has_label(Label label);
			int get_label(Label label);

		public:

			BufferWriter& label(Label label);

			void put_ascii(const std::string& str);
			void put_byte(uint8_t byte = 0);
			void put_byte(std::initializer_list<uint8_t> byte);
			void put_word(uint16_t word = 0);
			void put_word(std::initializer_list<uint16_t> word);
			void put_dword(uint32_t dword = 0);
			void put_dword(std::initializer_list<uint32_t> dword);
			void put_dword_f(float dword);
			void put_dword_f(std::initializer_list<float> dword);
			void put_qword(uint64_t dword = 0);
			void put_qword(std::initializer_list<uint64_t> dword);
			void put_qword_f(double dword);
			void put_qword_f(std::initializer_list<double> dword);

			// general
			void put_mov(Location dst, Location src);   /// Move
			void put_movsx(Location dst, Location src); /// Move with Sign Extension
			void put_movzx(Location dst, Location src); /// Move with Zero Extension
			void put_lea(Location dst, Location src);   /// Load Effective Address
			void put_xchg(Location dst, Location src);  /// Exchange
			void put_push(Location src);                /// Push
			void put_pop(Location src);                 /// Pop
			void put_inc(Location dst);                 /// Increment
			void put_dec(Location dst);                 /// Decrement
			void put_neg(Location dst);                 /// Negate
			void put_add(Location dst, Location src);   /// Add
			void put_adc(Location dst, Location src);   /// Add with carry
			void put_sub(Location dst, Location src);   /// Subtract
			void put_sbb(Location dst, Location src);   /// Subtract with borrow
			void put_cmp(Location dst, Location src);   /// Compare
			void put_and(Location dst, Location src);   /// Binary And
			void put_or(Location dst, Location src);    /// Binary Or
			void put_xor(Location dst, Location src);   /// Binary Xor
			void put_bt(Location dst, Location src);    /// Bit Test
			void put_bts(Location dst, Location src);   /// Bit Test and Set
			void put_btr(Location dst, Location src);   /// Bit Test and Reset
			void put_btc(Location dst, Location src);   /// Bit Test and Complement
			void put_mul(Location src);                 /// Multiply (Unsigned)
			void put_imul(Location dst, Location src);  /// Integer multiply (Signed)
			void put_imul(Location dst, Location src, Location val); /// Integer multiply (Signed, Triple Arg)
			void put_div(Location src);                 /// Divide (Unsigned)
			void put_idiv(Location src);                /// Integer divide (Signed)
			void put_not(Location dst);                 /// Invert
			void put_rol(Location dst, Location src);   /// Rotate Left
			void put_ror(Location dst, Location src);   /// Rotate Right
			void put_rcl(Location dst, Location src);   /// Rotate Left Through Carry
			void put_rcr(Location dst, Location src);   /// Rotate Right Through Carry
			void put_shl(Location dst, Location src);   /// Shift Left
			void put_shr(Location dst, Location src);   /// Shift Right
			void put_sal(Location dst, Location src);   /// Arithmetic Shift Left
			void put_sar(Location dst, Location src);   /// Arithmetic Shift Right
			void put_jmp(Location dst);                 /// Unconditional Jump
			void put_call(Location dst);                /// Procedure Call
			Label put_jo(Label label);                  /// Jump on Overflow
			Label put_jno(Label label);                 /// Jump on Not Overflow
			Label put_jb(Label label);                  /// Jump on Below
			Label put_jnb(Label label);                 /// Jump on Not Below
			Label put_je(Label label);                  /// Jump on Equal
			Label put_jne(Label label);                 /// Jump on Not Equal
			Label put_jbe(Label label);                 /// Jump on Below or Equal
			Label put_jnbe(Label label);                /// Jump on Not Below or Equal
			Label put_js(Label label);                  /// Jump on Sign
			Label put_jns(Label label);                 /// Jump on Not Sign
			Label put_jp(Label label);                  /// Jump on Parity
			Label put_jnp(Label label);                 /// Jump on Not Parity
			Label put_jl(Label label);                  /// Jump on Less
			Label put_jnl(Label label);                 /// Jump on Not Less
			Label put_jle(Label label);                 /// Jump on Less or Equal
			Label put_jnle(Label label);                /// Jump on Not Less or Equal
			Label put_jc(Label label);                  /// Alias to JB, Jump on Carry
			Label put_jnc(Label label);                 /// Alias to JNB, Jump on Not Carry
			Label put_jnae(Label label);                /// Alias to JB, Jump on Not Above or Equal
			Label put_jae(Label label);                 /// Alias to JNB, Jump on Above or Equal
			Label put_jz(Label label);                  /// Alias to JE, Jump on Zero
			Label put_jnz(Label label);                 /// Alias to JNE, Jump on Not Zero
			Label put_jna(Label label);                 /// Alias to JBE, Jump on Not Above
			Label put_ja(Label label);                  /// Alias to JNBE, Jump on Above
			Label put_jpe(Label label);                 /// Alias to JP, Jump on Parity Even
			Label put_jpo(Label label);                 /// Alias to JNP, Jump on Parity Odd
			Label put_jnge(Label label);                /// Alias to JL, Jump on Not Greater or Equal
			Label put_jge(Label label);                 /// Alias to JNL, Jump on Greater or Equal
			Label put_jng(Label label);                 /// Alias to JLE, Jump on Not Greater
			Label put_jg(Label label);                  /// Alias to JNLE, Jump on Greater
			Label put_jcxz(Label label);                /// Jump on CX Zero
			Label put_jecxz(Label label);               /// Jump on ECX Zero
			Label put_loop(Label label);                /// Loop Times
			void put_seto(Location dst);                /// Set Byte on Overflow
			void put_setno(Location dst);               /// Set Byte on Not Overflow
			void put_setb(Location dst);                /// Set Byte on Below
			void put_setnb(Location dst);               /// Set Byte on Not Below
			void put_sete(Location dst);                /// Set Byte on Equal
			void put_setne(Location dst);               /// Set Byte on Not Equal
			void put_setbe(Location dst);               /// Set Byte on Below or Equal
			void put_setnbe(Location dst);              /// Set Byte on Not Below or Equal
			void put_sets(Location dst);                /// Set Byte on Sign
			void put_setns(Location dst);               /// Set Byte on Not Sign
			void put_setp(Location dst);                /// Set Byte on Parity
			void put_setnp(Location dst);               /// Set Byte on Not Parity
			void put_setl(Location dst);                /// Set Byte on Less
			void put_setnl(Location dst);               /// Set Byte on Not Less
			void put_setle(Location dst);               /// Set Byte on Less or Equal
			void put_setnle(Location dst);              /// Set Byte on Not Less or Equal
			void put_setc(Location dst);                /// Alias to JB, Jump on Carry
			void put_setnc(Location dst);               /// Alias to JNB, Jump on Not Carry
			void put_setnae(Location dst);              /// Alias to JB, Jump on Not Above or Equal
			void put_setae(Location dst);               /// Alias to JNB, Jump on Above or Equal
			void put_setz(Location dst);                /// Alias to JE, Jump on Zero
			void put_setnz(Location dst);               /// Alias to JNE, Jump on Not Zero
			void put_setna(Location dst);               /// Alias to JBE, Jump on Not Above
			void put_seta(Location dst);                /// Alias to JNBE, Jump on Above
			void put_setpe(Location dst);               /// Alias to JP, Jump on Parity Even
			void put_setpo(Location dst);               /// Alias to JNP, Jump on Parity Odd
			void put_setnge(Location dst);              /// Alias to JL, Jump on Not Greater or Equal
			void put_setge(Location dst);               /// Alias to JNL, Jump on Greater or Equal
			void put_setng(Location dst);               /// Alias to JLE, Jump on Not Greater
			void put_setg(Location dst);                /// Alias to JNLE, Jump on Greater
			void put_int(Location type);                /// Interrupt
			void put_nop();                             /// No Operation
			void put_hlt();                             /// Halt
			void put_wait();                            /// Wait
			void put_leave();                           /// Leave
			void put_pusha();                           /// Push All
			void put_popa();                            /// Pop All
			void put_pushf();                           /// Push Flags
			void put_popf();                            /// Pop Flags
			void put_clc();                             /// Clear Carry
			void put_stc();                             /// Set Carry
			void put_cmc();                             /// Complement Carry Flag
			void put_sahf();                            /// Store AH into flags
			void put_lahf();                            /// Load status flags into AH register
			void put_aaa();                             /// ASCII adjust for add
			void put_daa();                             /// Decimal adjust for add
			void put_aas();                             /// ASCII adjust for subtract
			void put_das();                             /// Decimal adjust for subtract
			void put_aad();                             /// ASCII adjust for division
			void put_aam();                             /// ASCII adjust for multiplication
			void put_cbw();                             /// Convert byte to word
			void put_cwd();                             /// Convert word to double word
			void put_ret(uint16_t bytes = 0);           /// Return from procedure

			// floating-point
			void put_fnop();                            /// No Operation
			void put_finit();                           /// Initialize FPU
			void put_fninit();                          /// Initialize FPU (without checking for pending unmasked exceptions)
			void put_fclex();                           /// Clear Exceptions
			void put_fnclex();                          /// Clear Exceptions (without checking for pending unmasked exceptions)
			void put_fstsw(Location dst);               /// Store FPU State Word
			void put_fnstsw(Location dst);              /// Store FPU State Word (without checking for pending unmasked exceptions)
			void put_fstcw(Location dst);               /// Store FPU Control Word
			void put_fnstcw(Location dst);              /// Store FPU Control Word (without checking for pending unmasked exceptions)
			void put_fld1();                            /// Load +1.0 Constant onto the stack
			void put_fld0();                            /// Load +0.0 Constant onto the stack
			void put_fldpi();                           /// Load PI Constant onto the stack
			void put_fldl2t();                          /// Load Log{2}(10) Constant onto the stack
			void put_fldl2e();                          /// Load Log{2}(e) Constant onto the stack
			void put_fldlt2();                          /// Load Log{10}(2) Constant onto the stack
			void put_fldle2();                          /// Load Log{e}(2) Constant onto the stack
			void put_fldcw(Location src);               /// Load x87 FPU Control Word
			void put_f2xm1();                           /// Compute 2^x - 1
			void put_fabs();                            /// Absolute Value
			void put_fchs();                            /// Change Sign
			void put_fcos();                            /// Compute Cosine
			void put_fsin();                            /// Compute Sine
			void put_fsincos();                         /// Compute Sine and Cosine, sets ST(0) to sin(ST(0)), and pushes cos(ST(0)) onto the stack
			void put_fdecstp();                         /// Decrement Stack Pointer
			void put_fincstp();                         /// Increment Stack Pointer
			void put_fpatan();                          /// Partial Arctangent, sets ST(1) to arctan(ST(1) / ST(0)), and pops the stack
			void put_fprem();                           /// Partial Remainder, sets ST(0) to ST(0) % ST(1)
			void put_fprem1();                          /// Partial IEEE Remainder, sets ST(0) to ST(0) % ST(1)
			void put_fptan();                           /// Partial Tangent, sets ST(0) to tan(ST(0)), and pushes 1 onto the stack
			void put_frndint();                         /// Round to Integer, Rounds ST(0) to an integer
			void put_fscale();                          /// Scale, ST(0) by ST(1), Sets ST(0) to ST(0)*2^floor(ST(1))
			void put_fsqrt();                           /// Square Root, sets ST(0) to sqrt(ST(0))
			void put_fld(Location src);                 /// Load Floating-Point Value
			void put_fild(Location src);                /// Load Integer Value
			void put_fst(Location dst);                 /// Store Floating-Point Value
			void put_fstp(Location dst);                /// Store Floating-Point Value and Pop
			void put_fist(Location dst);                /// Store Floating-Point Value As Integer
			void put_fistp(Location dst);               /// Store Floating-Point Value As Integer And Pop
			void put_fisttp(Location dst);              /// Store Floating-Point Value As Integer With Truncation And Pop
			void put_ffree(Location src);               /// Free Floating-Point Register
			void put_fcmovb(Location src);              /// Move SRC, if below, into ST+0
			void put_fcmove(Location src);              /// Move SRC, if equal, into ST+0
			void put_fcmovbe(Location src);             /// Move SRC, if below or equal, into ST+0
			void put_fcmovu(Location src);              /// Move SRC, if unordered with, into ST+0
			void put_fcmovnb(Location src);             /// Move SRC, if not below, into ST+0
			void put_fcmovne(Location src);             /// Move SRC, if not equal, into ST+0
			void put_fcmovnbe(Location src);            /// Move SRC, if not below or equal, into ST+0
			void put_fcmovnu(Location src);             /// Move SRC, if not unordered with, into ST+0
			void put_fcom(Location src);                /// Compare ST+0 with SRC
			void put_fcomp(Location src);               /// Compare ST+0 with SRC And Pop
			void put_fcompp();                          /// Compare ST+0 with ST+1 And Pop Both
			void put_ficom(Location src);               /// Compare ST+0 with Integer SRC
			void put_ficomp(Location src);              /// Compare ST+0 with Integer SRC, And Pop
			void put_fcomi(Location src);               /// Compare ST+0 with SRC and set EFLAGS
			void put_fcomip(Location src);              /// Compare ST+0 with SRC, Pop, and set EFLAGS
			void put_fucomi(Location src);              /// Compare, and check for ordered values, ST+0 with SRC and set EFLAGS
			void put_fucomip(Location src);             /// Compare, and check for ordered values, ST+0 with SRC, Pop, and set EFLAGS
			void put_fmul(Location src);                /// Multiply By Memory Float
			void put_fimul(Location src);               /// Multiply By Memory Integer
			void put_fmul(Location dst, Location src);  /// Multiply
			void put_fmulp(Location dst);               /// Multiply And Pop
			void put_fadd(Location src);                /// Add Memory Float
			void put_fiadd(Location src);               /// Add Memory Integer
			void put_fadd(Location dst, Location src);  /// Add
			void put_faddp(Location dst);               /// Add And Pop
			void put_fdiv(Location src);                /// Divide By Memory Float
			void put_fidiv(Location src);               /// Divide By Memory Integer
			void put_fdiv(Location dst, Location src);  /// Divide
			void put_fdivp(Location dst);               /// Divide And Pop
			void put_fdivr(Location src);               /// Divide Memory Float
			void put_fidivr(Location src);              /// Divide Memory Integer
			void put_fdivr(Location dst, Location src); /// Reverse Divide
			void put_fdivrp(Location dst);              /// Reverse Divide And Pop

			void dump(bool verbose = false) const;
			void assemble(size_t absolute, bool debug = false);
			ExecutableBuffer bake(bool debug = false);
			ElfBuffer bake_elf(bool debug = false);

	};

}