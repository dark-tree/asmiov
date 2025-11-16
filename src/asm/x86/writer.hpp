#pragma once

#include "external.hpp"
#include "argument/location.hpp"
#include "out/buffer/segmented.hpp"
#include "../util.hpp"
#include "out/buffer/writer.hpp"

namespace asmio::x86 {

	enum LinkType {
		RELATIVE,
		ABSOLUTE,
	};

	class BufferWriter : public BasicBufferWriter {

		private:

			// number of bytes after the 'standard' instruction body
			// this is needed for the x86-64 RIP-relative addressing to work
			uint32_t suffix = 0;

			void put_linker_command(const Label& label, int32_t addend, int32_t shift, uint8_t width, LinkType type);
			void put_inst_rex(bool w, bool r, bool x, bool b);
			uint8_t pack_opcode_dw(uint8_t opcode, bool d, bool w);
			void put_inst_mod_reg_rm(uint8_t mod, uint8_t reg, uint8_t r_m);
			void put_inst_sib(uint8_t ss, uint8_t index, uint8_t base);
			void put_inst_imm(uint64_t immediate, uint8_t width);
			void put_inst_label_imm(Location imm, uint8_t size);

			/// Encode a 'standard' ModRM/SIB instruction with REX/size prefixes
			void put_inst_std(uint8_t opcode, const Location& dst, RegInfo packed, uint8_t size, bool longer = false);
			void put_inst_std_ri(uint8_t opcode, const Location& dst, uint8_t inst);
			void put_inst_std_as(uint8_t opcode, const Location& dst, RegInfo packed, bool longer = false);
			void put_inst_std_dw(uint8_t opcode, const Location& dst, RegInfo packed, uint8_t size, bool direction, bool wide, bool longer = false);

			/// Encode a directional instruction and deduce the 'wide' flag
			void put_inst_std_ds(uint8_t opcode, const Location& dst, RegInfo packed, uint8_t size, bool direction, bool longer = false);

			/// Encode many simple FPU instructions
			void put_inst_fpu(uint8_t opcode, uint8_t base, uint8_t sti = 0);

			/// Used for constructing the MOV instruction
			void put_inst_mov(const Location& dst, const Location& src, bool direction);

			/// Used for constructing the MOVSX and MOVZX instructions
			void put_inst_movx(uint8_t opcode, const Location& dst, const Location& src);

			/// Used to for constructing the shift instructions
			void put_inst_shift(const Location& dst, const Location& src, uint8_t inst);

			/// Used to for constructing the double shift instructions
			void put_inst_double_shift(uint8_t opcode, const Location& dst, const Location& src, const Location& cnt);

			/// Used for constructing a range of two argument instructions
			void put_inst_tuple(const Location& dst, const Location& src, uint8_t opcode_rmr, uint8_t opcode_reg);

			/// Used for constructing the Bit Test family of instructions
			void put_inst_btx(const Location& dst, const Location& src, uint8_t opcode, uint8_t inst);

			/// Used for constructing the conditional jump family of instructions
			void put_inst_jx(const Location& label, uint8_t sopcode, uint8_t lopcode);

			/// Used for constructing the 'set byte' family of instructions
			void put_inst_setx(const Location& dst, uint8_t lopcode);

			/// Add the REX.W prefix
			void put_rex_w();

			/// Override the operand size from 32 to 16 bit, don't use in combination with REX.W
			void put_16bit_operand_prefix();

			/// Override the default address size of 64 bit to 32 bit, don't use in combination with REX.W
			void put_32bit_address_prefix();

			void put_label(const Label& label, uint8_t size, int64_t addend);
			void set_suffix(int suffix);
			int get_suffix();

		public:

			BufferWriter(SegmentedBuffer& buffer);

			// string (i386)
			PREFIX put_rep();                           ///< Repeat
			PREFIX put_repe();                          ///< Repeat while equal
			PREFIX put_repz();                          ///< Repeat while zero
			PREFIX put_repne();                         ///< Repeat while not equal
			PREFIX put_repnz();                         ///< Repeat while not zero
			INST put_movsb();                           ///< Move byte from [ESI] to [EDI]
			INST put_movsw();                           ///< Move word from [ESI] to [EDI]
			INST put_movsd();                           ///< Move dword from [ESI] to [EDI]
			INST put_insb();                            ///< Input byte from I/O port specified in DX into [EDI]
			INST put_insw();                            ///< Input word from I/O port specified in DX into [EDI]
			INST put_insd();                            ///< Input dword from I/O port specified in DX into [EDI]
			INST put_outsb();                           ///< Output byte from [ESI] to I/O port specified in DX
			INST put_outsw();                           ///< Output word from [ESI] to I/O port specified in DX
			INST put_outsd();                           ///< Output dword from [ESI] to I/O port specified in DX
			INST put_cmpsb();                           ///< Compares byte at [ESI] with byte at [EDI]
			INST put_cmpsw();                           ///< Compares word at [ESI] with word at [EDI]
			INST put_cmpsd();                           ///< Compares dword at [ESI] with dword at [EDI]
			INST put_scasb();                           ///< Compare AL with byte at [EDI]
			INST put_scasw();                           ///< Compare AX with word at [EDI]
			INST put_scasd();                           ///< Compare EAX with dword at [EDI]
			INST put_lodsb();                           ///< Load byte at [ESI] into AL
			INST put_lodsw();                           ///< Load word at [ESI] into AX
			INST put_lodsd();                           ///< Load dword at [ESI] into EAX
			INST put_stosb();                           ///< Store byte AL at address [EDI]
			INST put_stosw();                           ///< Store word AX at address [EDI]
			INST put_stosd();                           ///< Store dword EAX at address [EDI]

			// general (i386)
			INST put_mov(Location dst, Location src);   ///< Move
			INST put_movsx(Location dst, Location src); ///< Move with Sign Extension
			INST put_movzx(Location dst, Location src); ///< Move with Zero Extension
			INST put_lea(Location dst, Location src);   ///< Load Effective Address
			INST put_xchg(Location dst, Location src);  ///< Exchange
			INST put_push(Location src);                ///< Push
			INST put_pop(Location src);                 ///< Pop
			INST put_pop();                             ///< Pop & Discard
			INST put_inc(Location dst);                 ///< Increment
			INST put_dec(Location dst);                 ///< Decrement
			INST put_neg(Location dst);                 ///< Negate
			INST put_add(Location dst, Location src);   ///< Add
			INST put_adc(Location dst, Location src);   ///< Add with carry
			INST put_sub(Location dst, Location src);   ///< Subtract
			INST put_sbb(Location dst, Location src);   ///< Subtract with borrow
			INST put_cmp(Location dst, Location src);   ///< Compare
			INST put_and(Location dst, Location src);   ///< Binary And
			INST put_or(Location dst, Location src);    ///< Binary Or
			INST put_xor(Location dst, Location src);   ///< Binary Xor
			INST put_bt(Location dst, Location src);    ///< Bit Test
			INST put_bts(Location dst, Location src);   ///< Bit Test and Set
			INST put_btr(Location dst, Location src);   ///< Bit Test and Reset
			INST put_btc(Location dst, Location src);   ///< Bit Test and Complement
			INST put_bsf(Location dst, Location src);   ///< Bit Scan Forward
			INST put_bsr(Location dst, Location src);   ///< Bit Scan Reverse
			INST put_mul(Location src);                 ///< Multiply (Unsigned)
			INST put_imul(Location dst, Location src);  ///< Integer multiply (Signed)
			INST put_imul(Location dst, Location src, Location val); ///< Integer multiply (Signed, Triple Arg)
			INST put_div(Location src);                 ///< Divide (Unsigned)
			INST put_idiv(Location src);                ///< Integer divide (Signed)
			INST put_not(Location dst);                 ///< Invert
			INST put_rol(Location dst, Location src);   ///< Rotate Left
			INST put_ror(Location dst, Location src);   ///< Rotate Right
			INST put_rcl(Location dst, Location src);   ///< Rotate Left Through Carry
			INST put_rcr(Location dst, Location src);   ///< Rotate Right Through Carry
			INST put_shl(Location dst, Location src);   ///< Shift Left
			INST put_shr(Location dst, Location src);   ///< Shift Right
			INST put_sal(Location dst, Location src);   ///< Arithmetic Shift Left
			INST put_sar(Location dst, Location src);   ///< Arithmetic Shift Right
			INST put_shld(Location dst, Location src, Location cnt); ///< Double Left Shift
			INST put_shrd(Location dst, Location src, Location cnt); ///< Double Right Shift
			INST put_jmp(Location dst);                 ///< Unconditional Jump
			INST put_call(Location dst);                ///< Procedure Call
			INST put_jo(Location label);                ///< Jump on Overflow
			INST put_jno(Location label);               ///< Jump on Not Overflow
			INST put_jb(Location label);                ///< Jump on Below
			INST put_jnb(Location label);               ///< Jump on Not Below
			INST put_je(Location label);                ///< Jump on Equal
			INST put_jne(Location label);               ///< Jump on Not Equal
			INST put_jbe(Location label);               ///< Jump on Below or Equal
			INST put_jnbe(Location label);              ///< Jump on Not Below or Equal
			INST put_js(Location label);                ///< Jump on Sign
			INST put_jns(Location label);               ///< Jump on Not Sign
			INST put_jp(Location label);                ///< Jump on Parity
			INST put_jnp(Location label);               ///< Jump on Not Parity
			INST put_jl(Location label);                ///< Jump on Less
			INST put_jnl(Location label);               ///< Jump on Not Less
			INST put_jle(Location label);               ///< Jump on Less or Equal
			INST put_jnle(Location label);              ///< Jump on Not Less or Equal
			INST put_jc(Location label);                ///< Alias to JB, Jump on Carry
			INST put_jnc(Location label);               ///< Alias to JNB, Jump on Not Carry
			INST put_jnae(Location label);              ///< Alias to JB, Jump on Not Above or Equal
			INST put_jae(Location label);               ///< Alias to JNB, Jump on Above or Equal
			INST put_jz(Location label);                ///< Alias to JE, Jump on Zero
			INST put_jnz(Location label);               ///< Alias to JNE, Jump on Not Zero
			INST put_jna(Location label);               ///< Alias to JBE, Jump on Not Above
			INST put_ja(Location label);                ///< Alias to JNBE, Jump on Above
			INST put_jpe(Location label);               ///< Alias to JP, Jump on Parity Even
			INST put_jpo(Location label);               ///< Alias to JNP, Jump on Parity Odd
			INST put_jnge(Location label);              ///< Alias to JL, Jump on Not Greater or Equal
			INST put_jge(Location label);               ///< Alias to JNL, Jump on Greater or Equal
			INST put_jng(Location label);               ///< Alias to JLE, Jump on Not Greater
			INST put_jg(Location label);                ///< Alias to JNLE, Jump on Greater
			INST put_jcxz(Location label);              ///< Jump on CX Zero
			INST put_jecxz(Location label);             ///< Jump on ECX Zero
			INST put_loop(Location label);              ///< Loop RCX Times
			INST put_loope(Location label);             ///< Loop RCX Times, if equal
			INST put_loopz(Location label);             ///< Loop RCX Times, if zero
			INST put_loopne(Location label);            ///< Loop RCX Times, if not equal
			INST put_loopnz(Location label);            ///< Loop RCX Times, if not zero
			INST put_seto(Location dst);                ///< Set Byte on Overflow
			INST put_setno(Location dst);               ///< Set Byte on Not Overflow
			INST put_setb(Location dst);                ///< Set Byte on Below
			INST put_setnb(Location dst);               ///< Set Byte on Not Below
			INST put_sete(Location dst);                ///< Set Byte on Equal
			INST put_setne(Location dst);               ///< Set Byte on Not Equal
			INST put_setbe(Location dst);               ///< Set Byte on Below or Equal
			INST put_setnbe(Location dst);              ///< Set Byte on Not Below or Equal
			INST put_sets(Location dst);                ///< Set Byte on Sign
			INST put_setns(Location dst);               ///< Set Byte on Not Sign
			INST put_setp(Location dst);                ///< Set Byte on Parity
			INST put_setnp(Location dst);               ///< Set Byte on Not Parity
			INST put_setl(Location dst);                ///< Set Byte on Less
			INST put_setnl(Location dst);               ///< Set Byte on Not Less
			INST put_setle(Location dst);               ///< Set Byte on Less or Equal
			INST put_setnle(Location dst);              ///< Set Byte on Not Less or Equal
			INST put_setc(Location dst);                ///< Alias to JB, Jump on Carry
			INST put_setnc(Location dst);               ///< Alias to JNB, Jump on Not Carry
			INST put_setnae(Location dst);              ///< Alias to JB, Jump on Not Above or Equal
			INST put_setae(Location dst);               ///< Alias to JNB, Jump on Above or Equal
			INST put_setz(Location dst);                ///< Alias to JE, Jump on Zero
			INST put_setnz(Location dst);               ///< Alias to JNE, Jump on Not Zero
			INST put_setna(Location dst);               ///< Alias to JBE, Jump on Not Above
			INST put_seta(Location dst);                ///< Alias to JNBE, Jump on Above
			INST put_setpe(Location dst);               ///< Alias to JP, Jump on Parity Even
			INST put_setpo(Location dst);               ///< Alias to JNP, Jump on Parity Odd
			INST put_setnge(Location dst);              ///< Alias to JL, Jump on Not Greater or Equal
			INST put_setge(Location dst);               ///< Alias to JNL, Jump on Greater or Equal
			INST put_setng(Location dst);               ///< Alias to JLE, Jump on Not Greater
			INST put_setg(Location dst);                ///< Alias to JNLE, Jump on Greater
			INST put_int(Location type);                ///< Interrupt
			INST put_into();                            ///< Interrupt if Overflow
			INST put_iret();                            ///< Return from Interrupt
			INST put_nop();                             ///< No Operation
			INST put_hlt();                             ///< Halt
			INST put_wait();                            ///< Wait
			INST put_ud2();                             ///< Undefined Instruction
			INST put_enter(Location alc, Location nst); ///< Enter Procedure
			INST put_leave();                           ///< Leave Procedure
			INST put_pusha();                           ///< Push RBX, RBP, R12-R15
			INST put_popa();                            ///< Pop RBX, RBP, R12-R15
			INST put_pushfd();                          ///< Push EFLAGS
			INST put_popfd();                           ///< Pop EFLAGS
			INST put_pushf();                           ///< Push Flags
			INST put_popf();                            ///< Pop Flags
			INST put_clc();                             ///< Clear Carry Flag
			INST put_stc();                             ///< Set Carry Flag
			INST put_cmc();                             ///< Complement Carry Flag
			INST put_cld();                             ///< Clear Direction Flag
			INST put_std();                             ///< Set Direction Flag
			INST put_cli();                             ///< Clear Interrupt Flag
			INST put_sti();                             ///< Set Interrupt Flag
			INST put_scf(Location src);                 ///< Set Carry Flag to Immediate, ASMIOV extension
			INST put_sdf(Location src);                 ///< Set Direction Flag to Immediate, ASMIOV extension
			INST put_sif(Location src);                 ///< Set Interrupt Flag to Immediate, ASMIOV extension
			INST put_sahf();                            ///< Store AH into flags
			INST put_lahf();                            ///< Load status flags into AH register
			INST put_aaa();                             ///< ASCII adjust for add
			INST put_daa();                             ///< Decimal adjust for add
			INST put_aas();                             ///< ASCII adjust for subtract
			INST put_das();                             ///< Decimal adjust for subtract
			INST put_cbw();                             ///< Convert byte to word
			INST put_cwd();                             ///< Convert word to double word
			INST put_xlat();                            ///< Table Look-up Translation
			INST put_in(Location dst, Location src);    ///< Input from Port
			INST put_out(Location dst, Location src);   ///< Output to Port
			INST put_test(Location dst, Location src);  ///< Test For Bit Pattern
			INST put_test(Location src);                ///< Sets flags accordingly to the value of register given, ASMIOV extension
			INST put_ret();                             ///< Return from procedure
			INST put_ret(Location bytes);               ///< Return from procedure and pop X bytes

			// i486
			INST put_xadd(Location dst, Location src);  ///< Exchange and Add
			INST put_bswap(Location dst);               ///< Byte Swap
			INST put_invd();                            ///< Invalidate Internal Caches
			INST put_wbinvd();                          ///< Write Back and Invalidate Cache
			INST put_cmpxchg(Location dst, Location src); ///< Compare and Exchange

			// x86-64
			INST put_cqo();                             ///< Convert Doubleword to Quadword
			INST put_swapgs();                          ///< Swap GS Base Register
			INST put_rdmsr();                           ///< Read From Model Specific Register
			INST put_wrmsr();                           ///< Write to Model Specific Register
			INST put_syscall();                         ///< Fast System Call
			INST put_sysretl();                         ///< Return From Fast System Call into Long Mode
			INST put_sysretc();                         ///< Return From Fast System Call into Compatibility Mode

			// floating-point
			INST put_fnop();                            ///< No Operation
			INST put_finit();                           ///< Initialize FPU
			INST put_fninit();                          ///< Initialize FPU (without checking for pending unmasked exceptions)
			INST put_fclex();                           ///< Clear Exceptions
			INST put_fnclex();                          ///< Clear Exceptions (without checking for pending unmasked exceptions)
			INST put_fstsw(Location dst);               ///< Store FPU State Word
			INST put_fnstsw(Location dst);              ///< Store FPU State Word (without checking for pending unmasked exceptions)
			INST put_fstcw(Location dst);               ///< Store FPU Control Word
			INST put_fnstcw(Location dst);              ///< Store FPU Control Word (without checking for pending unmasked exceptions)
			INST put_fld1();                            ///< Load +1.0 Constant onto the stack
			INST put_fld0();                            ///< Load +0.0 Constant onto the stack
			INST put_fldpi();                           ///< Load PI Constant onto the stack
			INST put_fldl2t();                          ///< Load Log{2}(10) Constant onto the stack
			INST put_fldl2e();                          ///< Load Log{2}(e) Constant onto the stack
			INST put_fldlt2();                          ///< Load Log{10}(2) Constant onto the stack
			INST put_fldle2();                          ///< Load Log{e}(2) Constant onto the stack
			INST put_fldcw(Location src);               ///< Load x87 FPU Control Word
			INST put_f2xm1();                           ///< Compute 2^x - 1
			INST put_fabs();                            ///< Absolute Value
			INST put_fchs();                            ///< Change Sign
			INST put_fcos();                            ///< Compute Cosine
			INST put_fsin();                            ///< Compute Sine
			INST put_fsincos();                         ///< Compute Sine and Cosine, sets ST(0) to sin(ST(0)), and pushes cos(ST(0)) onto the stack
			INST put_fdecstp();                         ///< Decrement Stack Pointer
			INST put_fincstp();                         ///< Increment Stack Pointer
			INST put_fpatan();                          ///< Partial Arctangent, sets ST(1) to arctan(ST(1) / ST(0)), and pops the stack
			INST put_fprem();                           ///< Partial Remainder, sets ST(0) to ST(0) % ST(1)
			INST put_fprem1();                          ///< Partial IEEE Remainder, sets ST(0) to ST(0) % ST(1)
			INST put_fptan();                           ///< Partial Tangent, sets ST(0) to tan(ST(0)), and pushes 1 onto the stack
			INST put_frndint();                         ///< Round to Integer, Rounds ST(0) to an integer
			INST put_fscale();                          ///< Scale, ST(0) by ST(1), Sets ST(0) to ST(0)*2^floor(ST(1))
			INST put_fsqrt();                           ///< Square Root, sets ST(0) to sqrt(ST(0))
			INST put_fld(Location src);                 ///< Load Floating-Point Value
			INST put_fild(Location src);                ///< Load Integer Value
			INST put_fst(Location dst);                 ///< Store Floating-Point Value
			INST put_fstp(Location dst);                ///< Store Floating-Point Value and Pop
			INST put_fist(Location dst);                ///< Store Floating-Point Value As Integer
			INST put_fistp(Location dst);               ///< Store Floating-Point Value As Integer And Pop
			INST put_fisttp(Location dst);              ///< Store Floating-Point Value As Integer With Truncation And Pop
			INST put_ffree(Location src);               ///< Free Floating-Point Register
			INST put_fcmovb(Location src);              ///< Move SRC, if below, into ST+0
			INST put_fcmove(Location src);              ///< Move SRC, if equal, into ST+0
			INST put_fcmovbe(Location src);             ///< Move SRC, if below or equal, into ST+0
			INST put_fcmovu(Location src);              ///< Move SRC, if unordered with, into ST+0
			INST put_fcmovnb(Location src);             ///< Move SRC, if not below, into ST+0
			INST put_fcmovne(Location src);             ///< Move SRC, if not equal, into ST+0
			INST put_fcmovnbe(Location src);            ///< Move SRC, if not below or equal, into ST+0
			INST put_fcmovnu(Location src);             ///< Move SRC, if not unordered with, into ST+0
			INST put_fcom(Location src);                ///< Compare ST+0 with SRC
			INST put_fcomp(Location src);               ///< Compare ST+0 with SRC And Pop
			INST put_fcompp();                          ///< Compare ST+0 with ST+1 And Pop Both
			INST put_ficom(Location src);               ///< Compare ST+0 with Integer SRC
			INST put_ficomp(Location src);              ///< Compare ST+0 with Integer SRC, And Pop
			INST put_fcomi(Location src);               ///< Compare ST+0 with SRC and set EFLAGS
			INST put_fcomip(Location src);              ///< Compare ST+0 with SRC, Pop, and set EFLAGS
			INST put_fucomi(Location src);              ///< Compare, and check for ordered values, ST+0 with SRC and set EFLAGS
			INST put_fucomip(Location src);             ///< Compare, and check for ordered values, ST+0 with SRC, Pop, and set EFLAGS
			INST put_fmul(Location src);                ///< Multiply By Memory Float
			INST put_fimul(Location src);               ///< Multiply By Memory Integer
			INST put_fmul(Location dst, Location src);  ///< Multiply
			INST put_fmulp(Location dst);               ///< Multiply And Pop
			INST put_fadd(Location src);                ///< Add Memory Float
			INST put_fiadd(Location src);               ///< Add Memory Integer
			INST put_fadd(Location dst, Location src);  ///< Add
			INST put_faddp(Location dst);               ///< Add And Pop
			INST put_fdiv(Location src);                ///< Divide By Memory Float
			INST put_fidiv(Location src);               ///< Divide By Memory Integer
			INST put_fdiv(Location dst, Location src);  ///< Divide
			INST put_fdivp(Location dst);               ///< Divide And Pop
			INST put_fdivr(Location src);               ///< Divide Memory Float
			INST put_fidivr(Location src);              ///< Divide Memory Integer
			INST put_fdivr(Location dst, Location src); ///< Reverse Divide
			INST put_fdivrp(Location dst);              ///< Reverse Divide And Pop

	};

}
