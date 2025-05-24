
#define DEBUG_MODE false

#include "lib/vstl.hpp"
#include "asm/x86/writer.hpp"
#include "asm/x86/emitter.hpp"
#include "file/elf/buffer.hpp"
#include "tasml/tokenizer.hpp"
#include "tasml/stream.hpp"

// private libs
#include <fstream>

using namespace asmio::x86;

TEST (writer_push_check) {

	BufferWriter writer;

	// 16 bit
	writer.put_push(AX);
	writer.put_push(BX);
	writer.put_push(BP);
	writer.put_push(R9W);
	writer.put_push(cast<WORD>(ref(RAX)));

	// 64 bit
	writer.put_push(RAX);
	writer.put_push(RBX);
	writer.put_push(RBP);
	writer.put_push(R11);
	writer.put_push(R13);
	writer.put_push(R15);
	writer.put_push(cast<QWORD>(ref(RAX)));

	// 8 bit
	EXPECT_ANY({ writer.put_push(AL); });
	EXPECT_ANY({ writer.put_push(AH); });
	EXPECT_ANY({ writer.put_push(SPL); });
	EXPECT_ANY({ writer.put_push(cast<BYTE>(ref(RAX))); });

	// 32 bit
	EXPECT_ANY({ writer.put_push(EAX); });
	EXPECT_ANY({ writer.put_push(R10D); });
	EXPECT_ANY({ writer.put_push(R15D); });
	EXPECT_ANY({ writer.put_push(cast<DWORD>(ref(RAX))); });

	// too large imm
	EXPECT_ANY({ writer.put_push(0xffffffffff); });

}

TEST (switch_mode_rbp_protection) {

	BufferWriter writer;

	writer.put_push(0);
	writer.put_pop(RBP);
	writer.put_ret();

	writer.bake().call_u32();

}

TEST (writer_exec_mov_ret_nop) {

	BufferWriter writer;

	writer.put_mov(EAX, 5);
	writer.put_nop();
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	uint32_t eax = buffer.call_u32();

	CHECK(eax, 5);

}

TEST (writer_exec_mem_moves) {

	BufferWriter writer;

	writer.label("a").put_dword();
	writer.label("b").put_dword();
	writer.label("c").put_dword(34);

	writer.label("main");

	// test 1
	writer.put_mov(EAX, 12);
	writer.put_mov(ref("a"), EAX);
	writer.put_mov(EDX, ref("a"));
	writer.put_cmp(EDX, 12);
	writer.put_jne("fail");

	// test 2
	writer.put_mov(ref("b"), 121);
	writer.put_mov(EDX, ref("b"));
	writer.put_cmp(EDX, 121);
	writer.put_jne("fail");

	// test 3
	writer.put_mov(EDX, ref("c"));
	writer.put_cmp(EDX, 34);
	writer.put_jne("fail");

	writer.put_mov(EAX, 1);
	writer.put_ret();

	writer.label("fail");
	writer.put_mov(EAX, 0);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u32("main"), 1);

}

TEST (writer_exec_rol_inc_neg_sar) {

	BufferWriter writer;

	writer.put_nop();
	writer.put_mov(EDX, 5);
	writer.put_rol(EDX, 3);
	writer.put_inc(EDX);
	writer.put_mov(EAX, EDX);
	writer.put_inc(EAX);
	writer.put_neg(EAX);
	writer.put_mov(CL, 2);
	writer.put_sar(EAX, CL);
	writer.put_nop();
	writer.put_neg(EAX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	uint32_t eax = buffer.call_u32();

	CHECK(eax, 11);

}

TEST(writer_exec_xchg) {

	BufferWriter writer;

	writer.put_nop();
	writer.put_mov(EAX, 0xA);
	writer.put_mov(EDX, 0xD);
	writer.put_xchg(EDX, EAX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	uint32_t eax = buffer.call_u32();

	CHECK(eax, 0xD);

}

TEST(writer_exec_push_pop) {

	BufferWriter writer;

	writer.put_mov(EAX, 9);
	writer.put_push(EAX);
	writer.put_mov(EAX, 7);
	writer.put_pop(ECX);
	writer.put_mov(EAX, ECX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	uint32_t eax = buffer.call_u32();

	CHECK(eax, 9);

}

TEST(writer_exec_movsx_movzx) {

	BufferWriter writer;

	writer.put_mov(CL, 9);
	writer.put_movzx(EDX, CL);
	writer.put_neg(DL);
	writer.put_movsx(EAX, DL);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int eax = buffer.call_i32();

	CHECK(eax, -9);

}

TEST(writer_exec_lea) {

	int eax = 12, edx = 56, ecx = 60;

	BufferWriter writer;

	writer.put_mov(EAX, eax);
	writer.put_mov(EDX, edx);
	writer.put_mov(ECX, ecx);
	writer.put_lea(ECX, EDX + EAX * 2 - 5);
	writer.put_lea(EAX, ECX + EAX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call_i32();

	CHECK(output, (edx + eax * 2 - 5) + eax);
}

TEST(writer_exec_add) {

	int eax = 12, edx = 56, ecx = 60;

	BufferWriter writer;

	writer.put_mov(EAX, eax);
	writer.put_mov(EDX, edx);
	writer.put_mov(ECX, ecx);
	writer.put_add(ECX, EDX);
	writer.put_add(EAX, ECX);
	writer.put_add(EAX, 5);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call_i32();

	CHECK(output, eax + edx + ecx + 5);

}

TEST(writer_exec_add_adc) {

	BufferWriter writer;

	writer.put_mov(EAX, 0xFFFFFFF0);
	writer.put_mov(EDX, 0x00000000);
	writer.put_add(EAX, 0x1F);
	writer.put_adc(EDX, 0);
	writer.put_xchg(EAX, EDX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call_i32();

	CHECK(output, 0x1);

}

TEST(writer_exec_sub) {

	BufferWriter writer;

	writer.put_mov(EAX, 0xF);
	writer.put_mov(EDX, 0xA);
	writer.put_sub(EAX, EDX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call_i32();

	CHECK(output, 0xF - 0xA);

}

TEST(writer_exec_sub_sbb) {

	BufferWriter writer;

	writer.put_mov(EAX, 0xA);
	writer.put_mov(EDX, 0x0);
	writer.put_sub(EAX, 0xB);
	writer.put_sbb(EDX, 0);
	writer.put_xchg(EAX, EDX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call_i32();

	CHECK(output, -1);

}

TEST(writer_exec_stc_lahf_sahf) {

	BufferWriter writer;

	writer.put_mov(AH, 0);
	writer.put_sahf();
	writer.put_stc();
	writer.put_lahf();
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	uint32_t output = (buffer.call_u32() & 0xFF00) >> 8;

	ASSERT(!(output & 0b1000'0000)); // ZF
	ASSERT( (output & 0b0000'0001)); // CF
	ASSERT( (output & 0b0000'0010)); // always set to 1

}

TEST (writer_exec_cmp_lahf) {

	BufferWriter writer;

	writer.put_mov(EAX, 0xA);
	writer.put_mov(EDX, 0xE);
	writer.put_cmp(EAX, EDX);
	writer.put_lahf();
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	uint32_t output = (buffer.call_u32() & 0xFF00) >> 8;

	ASSERT(output & 0b1000'0000); // ZF
	ASSERT(output & 0b0000'0001); // CF
	ASSERT(output & 0b0000'0010); // always set to 1

}

TEST(writer_exec_mul) {

	BufferWriter writer;

	writer.put_mov(EAX, 0xA);
	writer.put_mov(EDX, 0xB);
	writer.put_mul(EDX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call_i32();

	CHECK(output, 0xA * 0xB);

}

TEST(writer_exec_mul_imul) {

	BufferWriter writer;

	writer.put_mov(EAX, 7);
	writer.put_mov(EDX, 5);
	writer.put_mov(ECX, 2);
	writer.put_mul(EDX);
	writer.put_imul(EAX, 3);
	writer.put_imul(EAX, ECX);
	writer.put_imul(EAX, EAX, 11);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call_i32();

	CHECK(output, 7 * 5 * 3 * 2 * 11);

}

TEST(writer_exec_div_idiv) {

	BufferWriter writer;

	writer.put_mov(EAX, 50);

	writer.put_mov(EDX, 0);
	writer.put_mov(ECX, 5);
	writer.put_div(ECX);

	writer.put_mov(EDX, 0);
	writer.put_mov(ECX, -2);
	writer.put_idiv(ECX);

	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call_i32();

	CHECK(output, 50 / 5 / -2);

}

TEST(writer_exec_or) {

	BufferWriter writer;

	writer.put_mov(EAX, 0b0000'0110);
	writer.put_mov(EDX, 0b0101'0000);
	writer.put_or(EAX,  0b0000'1100);
	writer.put_or(EAX, EDX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call_i32();

	CHECK(output, 0b0101'1110);

}

TEST(writer_exec_and_not_xor_or) {

	BufferWriter writer;

	writer.put_mov(EAX, 0b0000'0110'0111);
	writer.put_mov(EDX, 0b0101'0010'1010);
	writer.put_mov(ECX, 0b1011'0000'0110);

	writer.put_not(ECX);      // -> 0b0100'1111'1001
	writer.put_and(EDX, ECX); // -> 0b0100'0010'1000
	writer.put_xor(EAX, EDX); // -> 0b0100'0100'1111
	writer.put_or(EAX, 0b1000'0000'0001);

	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call_i32();

	CHECK(output, 0b1100'0100'1111);

}

TEST(writer_exec_aam) {

	BufferWriter writer;

	writer.put_mov(EAX, 27);
	writer.put_aam(); // AL -> 7, AH -> 2
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call_i32();

	CHECK(output, 0x0207);

}

TEST(writer_exec_aad) {

	BufferWriter writer;

	writer.put_mov(EAX, 0x0509);
	writer.put_aad(); // AL -> 59, AH -> 0
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call_i32();

	CHECK(output, 59);

}

TEST(writer_exec_bts_btr_btc) {

	BufferWriter writer;

	//                    7654 3210
	writer.put_mov(EAX, 0b1101'0001);

	writer.put_bts(EAX, 3);
	writer.put_bts(EAX, 6);
	writer.put_btr(EAX, 4);
	writer.put_btr(EAX, 5);
	writer.put_btc(EAX, 0);
	writer.put_btc(EAX, 1);

	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call_i32();

	CHECK(output, 0b1100'1010);

}

TEST(writer_exec_jmp_forward) {

	BufferWriter writer;

	writer.put_mov(EAX, 1);
	writer.put_jmp("l_skip");
	writer.put_mov(EAX, 2);
	writer.label("l_skip");
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call_i32();

	CHECK(output, 1);

}

TEST(writer_exec_jmp_back) {

	BufferWriter writer;

	writer.put_mov(EAX, 1);
	writer.put_jmp("1");
	writer.put_ret();

	writer.label("2");
	writer.put_mov(EAX, 3);
	writer.put_jmp("3");
	writer.put_ret();

	writer.label("1");
	writer.put_mov(EAX, 2);
	writer.put_jmp("2");
	writer.put_ret();

	writer.label("3");
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call_i32();

	CHECK(output, 3);

}

TEST(writer_exec_je) {

	BufferWriter writer;

	writer.put_mov(EAX, 5);
	writer.put_mov(ECX, 6);
	writer.put_cmp(ECX, 7);

	writer.put_jb("skip");
	writer.put_mov(EAX, 0);
	writer.label("skip");

	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call_i32();

	CHECK(output, 5);

}

TEST(writer_exec_labeled_entry) {

	BufferWriter writer;

	writer.label("foo");
	writer.put_mov(EAX, 1);
	writer.put_ret();

	writer.label("bar");
	writer.put_mov(EAX, 2);
	writer.put_ret();

	writer.label("tar");
	writer.put_mov(EAX, 3);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();

	CHECK(buffer.call_u32("foo"), 1);
	CHECK(buffer.call_u32("bar"), 2);
	CHECK(buffer.call_u32("tar"), 3);

}

TEST(writer_exec_functions) {

	BufferWriter writer;

	writer.label("add");
	writer.put_add(EAX, ref(ESP + 4));
	writer.put_ret();

	writer.label("main");
	writer.put_mov(EAX, 0);

	// add 20 to EAX
	writer.put_push(20);
	writer.put_call("add");
	writer.put_add(ESP, 4);

	// add 12 to EAX
	writer.put_push(12);
	writer.put_call("add");
	writer.put_add(ESP, 4);

	// add 10 to EAX
	writer.put_push(10);
	writer.put_call("add");
	writer.put_add(ESP, 4);

	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u32("main"), 42);

}

TEST(writer_exec_label_mov) {

	BufferWriter writer;

	writer.put_word(0x1234);

	writer.label("main");
	writer.put_mov(EDX, 12);
	writer.put_lea(EAX, EDX + "main");
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u32("main"), 14 + (size_t) buffer.address());

}

TEST(writer_exec_absolute_jmp) {

	BufferWriter writer;

	writer.put_word(0x1234);
	writer.put_mov(EAX, 0);
	writer.put_ret(); // invalid

	writer.label("back");
	writer.put_mov(EAX, 42);
	writer.put_ret(); // valid

	writer.label("main");
	writer.put_mov(EAX, 12);
	writer.put_mov(EDX, "back");
	writer.put_jmp(EDX);
	writer.put_ret(); // invalid

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u32("main"), 42);

}

TEST (writer_exec_fpu_fnop_finit_fld1) {

	BufferWriter writer;

	writer.put_fnop();
	writer.put_finit();
	writer.put_fld1();
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32(), 1.0f);

}

TEST (writer_exec_fpu_fmul_fimul_fmulp) {

	BufferWriter writer;

	writer.label("a");
	writer.put_dword_f(6.0f);

	writer.label("b");
	writer.put_dword_f(0.5f);

	writer.label("c");
	writer.put_dword(4);

	writer.label("d");
	writer.put_qword_f(0.25f);

	writer.label("main");
	writer.put_finit();
	writer.put_fld(ref("a"));              // fpu stack: [+6.0]
	writer.put_fld(ref("b"));              // fpu stack: [+0.5, +6.0]
	writer.put_fmul(ST + 0, ST + 1);       // fpu stack: [0.5*6.0, +6.0]
	writer.put_fimul(ref("c"));            // fpu stack: [0.5*6.0*4.0, +6.0]
	writer.put_fld(cast<QWORD>(ref("d"))); // fpu stack: [0.25, 0.5*6.0*4.0, +6.0]
	writer.put_fmulp(ST + 1);              // fpu stack: [0.5*6.0*4.0*0.25, +6.0]
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("main"), 3.0f);

}

TEST(writer_exec_fpu_f2xm1_fabs_fchs) {

	BufferWriter writer;

	writer.label("a");
	writer.put_dword_f(1.0f);

	writer.label("b");
	writer.put_dword_f(5.0f);

	writer.label("main");
	writer.put_fld(ref("b")); // fpu stack: [+5.0f]
	writer.put_fld(ref("a")); // fpu stack: [+1.0, +5.0f]
	writer.put_fchs();        // fpu stack: [-1.0, +5.0f]
	writer.put_f2xm1();       // fpu stack: [2^(-1.0)-1, +5.0f]
	writer.put_fmulp(ST + 1); // fpu stack: [(2^(-1.0)-1)*5.0]
	writer.put_fabs();        // fpu stack: [|(2^(-1.0)-1)*5.0|]
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("main"), 2.5f);

}

TEST (writer_exec_fpu_fadd_fiadd_faddp) {

	BufferWriter writer;

	writer.label("a");
	writer.put_dword_f(6.0f);

	writer.label("b");
	writer.put_dword_f(0.5f);

	writer.label("c");
	writer.put_dword(4);

	writer.label("d");
	writer.put_qword_f(0.25f);

	writer.label("main");
	writer.put_finit();
	writer.put_fld(ref("a"));              // fpu stack: [+6.0]
	writer.put_fld(ref("b"));              // fpu stack: [+0.5, +6.0]
	writer.put_fadd(ST + 0, ST + 1);       // fpu stack: [0.5+6.0, +6.0]
	writer.put_fiadd(ref("c"));            // fpu stack: [0.5+6.0+4.0, +6.0]
	writer.put_fld(cast<QWORD>(ref("d"))); // fpu stack: [0.25, 0.5+6.0+4.0, +6.0]
	writer.put_faddp(ST + 1);              // fpu stack: [0.5+6.0+4.0+0.25, +6.0]
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("main"), 10.75f);

}

TEST (writer_exec_fpu_fcom_fstsw_fcomp_fcmove_fcmovb) {

	BufferWriter writer;

	writer.label("a");
	writer.put_dword_f(6.0f);

	writer.label("b");
	writer.put_dword_f(0.5f);

	writer.label("c");
	writer.put_dword_f(4.0f);

	writer.label("main");
	writer.put_finit();
	writer.put_fld(ref("a"));  // fpu stack: [+6.0]
	writer.put_fld(ref("b"));  // fpu stack: [+0.5, +6.0]
	writer.put_fld(ref("c"));  // fpu stack: [+4.0, +0.5, +6.0]
	writer.put_fcom(ref("a")); // fpu stack: [+4.0, +0.5, +6.0]
	writer.put_fstsw(AX);
	writer.put_sahf();
	writer.put_fcmovb(ST + 2); // fpu stack: [+6.0, +0.5, +6.0]
	writer.put_fld(ref("c"));  // fpu stack: [+6.0, +6.0, +0.5, +6.0]
	writer.put_fcomp(ST + 2);  // fpu stack: [+6.0, +0.5, +6.0]
	writer.put_fstsw(AX);
	writer.put_sahf();
	writer.put_fcmove(ST + 1); // fpu stack: [+6.0, +0.5, +6.0]
	writer.put_faddp(ST + 1);  // fpu stack: [6.0+0.5, +6.0]
	writer.put_faddp(ST + 1);  // fpu stack: [6.0+0.5+6.0]
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("main"), 12.5f);

}

TEST (writer_exec_fpu_fcompp) {

	BufferWriter writer;

	writer.label("a");
	writer.put_dword_f(6.0f);

	writer.label("b");
	writer.put_dword_f(0.5f);

	writer.label("c");
	writer.put_dword_f(4.0f);

	writer.label("main");
	writer.put_finit();
	writer.put_fld(ref("a"));  // fpu stack: [+6.0]
	writer.put_fld(ref("b"));  // fpu stack: [+0.5, +6.0]
	writer.put_fld(ref("c"));  // fpu stack: [+4.0, +0.5, +6.0]
	writer.put_fcompp();       // fpu stack: [+6.0]
	writer.put_fstsw(AX);
	writer.put_sahf();
	writer.put_jb("invalid");
	writer.put_je("invalid");
	writer.put_ret();

	writer.label("invalid");
	writer.put_fld0();
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("main"), 6.0f);

}

TEST (writer_exec_fpu_fcomi_fcomip) {

	BufferWriter writer;

	writer.label("a");
	writer.put_dword_f(3.0f);

	writer.label("b");
	writer.put_dword_f(0.5f);

	writer.label("c");
	writer.put_dword_f(4.0f);

	writer.label("main");
	writer.put_finit();
	writer.put_fld(ref("b"));  // fpu stack: [+0.5]
	writer.put_fld(ref("c"));  // fpu stack: [+4.0, +0.5]
	writer.put_fcomi(ST + 1);  // fpu stack: [+4.0, +0.5]
	writer.put_jb("invalid");
	writer.put_je("invalid");
	writer.put_ja("main2");
	writer.put_jmp("invalid");

	writer.label("main2");
	writer.put_fld(ref("a"));  // fpu stack: [3.0f, +4.0, +0.5]
	writer.put_fcomip(ST + 2); // fpu stack: [+4.0, +0.5]
	writer.put_jb("invalid");
	writer.put_je("invalid");
	writer.put_ja("main3");
	writer.put_jmp("invalid");

	writer.label("main3");
	writer.put_faddp(ST + 1);  // fpu stack: [4.0+0.5]
	writer.put_ret();

	writer.label("invalid");
	writer.put_fld0();
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("main"), 4.5f);

}

TEST (writer_exec_fpu_fldpi_fcos) {

	BufferWriter writer;

	writer.label("main");
	writer.put_finit();
	writer.put_fldpi();        // fpu stack: [PI]
	writer.put_fcos();         // fpu stack: [cos(PI)]
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("main"), -1);

}

TEST (writer_exec_fpu_fdiv_fdivp) {

	BufferWriter writer;

	writer.label("a");
	writer.put_dword_f(2.0f);

	writer.label("b");
	writer.put_dword_f(8.0f);

	writer.label("c");
	writer.put_dword_f(0.125f);

	writer.label("main");
	writer.put_finit();
	writer.put_fld(ref("a"));        // fpu stack: [+2.0]
	writer.put_fld(ref("b"));        // fpu stack: [+8.0, +2.0]
	writer.put_fdiv(ST + 0, ST + 1); // fpu stack: [8.0/2.0, +2.0]
	writer.put_fld(ref("c"));        // fpu stack: [+0.125, 8.0/2.0, +2.0]
	writer.put_fdivp(ST + 2);        // fpu stack: [8.0/2.0, 2.0/0.125]
	writer.put_faddp(ST + 1);        // fpu stack: [8.0/2.0+2.0/0.125]
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("main"), 20);

}

TEST (writer_exec_fpu_fdivr_fdivrp) {

	BufferWriter writer;

	writer.label("a");
	writer.put_dword_f(2.0f);

	writer.label("c");
	writer.put_dword_f(12.0f);

	writer.label("main");
	writer.put_finit();
	writer.put_fld(ref("a"));         // fpu stack: [+2.0]
	writer.put_fld(ref("a"));         // fpu stack: [+2.0, +2.0]
	writer.put_fdivr(ST + 0, ST + 1); // fpu stack: [2.0/2.0, +2.0]
	writer.put_fld(ref("c"));         // fpu stack: [+12.0, 2.0/2.0, +2.0]
	writer.put_fdivrp(ST + 2);        // fpu stack: [2.0/2.0, 12.0/2.0]
	writer.put_faddp(ST + 1);         // fpu stack: [2.0/2.0+12.0/2.0]
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("main"), 7);

}

TEST (writer_exec_fpu_ficom_ficomp) {

	BufferWriter writer;

	writer.label("af");
	writer.put_dword_f(2.0f);

	writer.label("ai");
	writer.put_dword(2);

	writer.label("bf");
	writer.put_dword_f(3.0f);

	writer.label("bi");
	writer.put_dword(3);

	writer.label("main");
	writer.put_finit();
	writer.put_fld(ref("af"));    // fpu stack: [2.0]
	writer.put_ficom(ref("ai"));  // fpu stack: [2.0]
	writer.put_fstsw(AX);
	writer.put_sahf();
	writer.put_jne("invalid");
	writer.put_fld(ref("bf"));    // fpu stack: [3.0, 2.0]
	writer.put_ficomp(ref("bi")); // fpu stack: [2.0]
	writer.put_fstsw(AX);
	writer.put_sahf();
	writer.put_jne("invalid");
	writer.put_ficomp(ref("bi")); // fpu stack: []
	writer.put_fstsw(AX);
	writer.put_sahf();
	writer.put_je("invalid");
	writer.put_fld1();            // fpu stack: [1.0]
	writer.put_ret();

	writer.label("invalid");
	writer.put_fld0();
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("main"), 1);

}

TEST (writer_exec_fpu_fdecstp_fincstp) {

	BufferWriter writer;

	writer.put_finit();
	writer.put_fld1();
	writer.put_fdecstp();
	writer.put_fincstp();
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32(), 1);

}

TEST (writer_exec_fpu_fild) {

	BufferWriter writer;

	writer.label("a");
	writer.put_word(1);

	writer.label("b");
	writer.put_dword(2);

	writer.label("c");
	writer.put_qword(3);

	writer.label("main");
	writer.put_finit();
	writer.put_fild(cast<WORD>(ref("a")));  // fpu stack: [1.0]
	writer.put_fild(cast<DWORD>(ref("b"))); // fpu stack: [2.0, 1.0]
	writer.put_fild(cast<QWORD>(ref("c"))); // fpu stack: [3.0, 2.0, 1.0]
	writer.put_faddp(ST + 1);               // fpu stack: [3.0+2.0, 1.0]
	writer.put_faddp(ST + 1);               // fpu stack: [3.0+2.0+1.0]
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("main"), 6);

}

TEST (writer_exec_fpu_fist_fistp) {

	BufferWriter writer;

	writer.label("a");
	writer.put_dword(0);

	writer.label("b");
	writer.put_dword(0);

	writer.label("init");
	writer.put_finit();
	writer.put_fld1();          // fpu stack: [1.0]
	writer.put_fld1();          // fpu stack: [1.0, 1.0]
	writer.put_fld1();          // fpu stack: [1.0, 1.0, 1.0]
	writer.put_faddp(ST + 1);   // fpu stack: [2.0, 1.0]
	writer.put_fist(ref("a"));  // fpu stack: [2.0, 1.0]
	writer.put_fld1();          // fpu stack: [1.0, 2.0, 1.0]
	writer.put_fld1();          // fpu stack: [1.0, 1.0, 2.0, 1.0]
	writer.put_faddp(ST + 1);   // fpu stack: [2.0, 2.0, 1.0]
	writer.put_faddp(ST + 1);   // fpu stack: [4.0, 1.0]
	writer.put_fistp(ref("b")); // fpu stack: [1.0]
	writer.put_ret();

	writer.label("main");
	writer.put_mov(EAX, ref("a"));
	writer.put_add(EAX, ref("b"));
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("init"), 1.0f);
	CHECK(buffer.call_i32("main"), 6);

}

TEST (writer_exec_fpu_fisttp) {

	BufferWriter writer;

	writer.label("a");
	writer.put_dword_f(3.9);

	writer.label("b");
	writer.put_dword(0);

	writer.label("init");
	writer.put_finit();
	writer.put_fld1();           // fpu stack: [1.0]
	writer.put_fld(ref("a"));    // fpu stack: [3.9, 1.0]
	writer.put_fisttp(ref("b")); // fpu stack: [1.0]
	writer.put_ret();

	writer.label("main");
	writer.put_mov(EAX, ref("b"));
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("init"), 1.0f);
	CHECK(buffer.call_i32("main"), 3);

}

TEST (writer_exec_fpu_fldpi_frndint) {

	BufferWriter writer;

	writer.label("main");
	writer.put_finit();
	writer.put_fldpi();   // fpu stack: [3.1415]
	writer.put_frndint(); // fpu stack: [3.0]
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("main"), 3.0f);

}

TEST (writer_exec_fpu_fsqrt) {

	BufferWriter writer;

	writer.label("a");
	writer.put_dword_f(16.0f);

	writer.label("main");
	writer.put_finit();
	writer.put_fld(ref("a")); // fpu stack: [16.0]
	writer.put_fsqrt();       // fpu stack: [4.0]
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("main"), 4.0f);

}

TEST (writer_exec_fpu_fst) {

	BufferWriter writer;

	writer.label("dword_a");
	writer.put_dword_f(0); // => 2

	writer.label("dword_b");
	writer.put_dword_f(0); // => 3

	writer.label("qword_c");
	writer.put_qword_f(0); // => 6

	writer.label("set_a");
	writer.put_finit();
	writer.put_fld1();                            // fpu stack: [1.0]
	writer.put_fld1();                            // fpu stack: [1.0, 1.0]
	writer.put_faddp(ST + 1);                     // fpu stack: [2.0]
	writer.put_fst(ref("dword_a"));               // fpu stack: [2.0]
	writer.put_ret();

	writer.label("set_b");
	writer.put_finit();
	writer.put_fld1();                            // fpu stack: [1.0]
	writer.put_fld1();                            // fpu stack: [1.0, 1.0]
	writer.put_fld(ref("dword_a"));               // fpu stack: [2.0, 1.0, 1.0]
	writer.put_faddp(ST + 1);                     // fpu stack: [3.0, 1.0]
	writer.put_fstp(ref("dword_b"));              // fpu stack: [1.0]
	writer.put_ret();

	writer.label("set_c");
	writer.put_finit();
	writer.put_fld0();                            // fpu stack: [0.0]
	writer.put_fld(ref("dword_a"));               // fpu stack: [2.0, 0.0]
	writer.put_fld(ref("dword_b"));               // fpu stack: [3.0, 2.0, 0.0]
	writer.put_fst(ST + 1);                       // fpu stack: [3.0, 3.0, 0.0]
	writer.put_faddp(ST + 1);                     // fpu stack: [6.0, 0.0]
	writer.put_fstp(cast<TWORD>(ref("qword_c"))); // fpu stack: [0.0]
	writer.put_ret();

	writer.label("ctrl_sum");
	writer.put_finit();
	writer.put_fld(ref("dword_a"));               // fpu stack: [2.0]
	writer.put_fld(ref("dword_b"));               // fpu stack: [3.0, 2.0]
	writer.put_fld(cast<TWORD>(ref("qword_c")));  // fpu stack: [6.0, 3.0, 2.0]
	writer.put_faddp(ST + 1);                     // fpu stack: [9.0, 2.0]
	writer.put_faddp(ST + 1);                     // fpu stack: [11.0]
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("set_a"), 2.0f);
	CHECK(buffer.call_f32("set_b"), 1.0f);
	CHECK(buffer.call_f32("set_c"), 0.0f);
	CHECK(buffer.call_f32("ctrl_sum"), 11.0f);

}

TEST (writer_exec_bt_dec) {

	BufferWriter writer;

	writer.label("var").put_dword();

	writer.label("main");
	writer.put_mov(ref("var"), 17); // 10001
	writer.put_dec(ref("var"));     // 10000

	writer.put_bt(ref("var"), 4);
	writer.put_jnc("invalid_1");

	writer.put_bt(ref("var"), 0);
	writer.put_jc("invalid_2");

	writer.put_mov(EAX, 0);
	writer.put_ret();

	writer.label("invalid_1");
	writer.put_mov(EAX, 1);
	writer.put_ret();

	writer.label("invalid_2");
	writer.put_mov(EAX, 2);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_i32("main"), 0);

}

TEST (writer_exec_setx) {

	BufferWriter writer;

	writer.label("db").put_byte(0);
	writer.label("dw").put_word(0);
	writer.label("dd").put_dword(0);

	writer.label("main");
	writer.put_mov(EAX, 0);
	writer.put_stc();
	writer.put_setc(EAX);
	writer.put_setc(cast<BYTE>(ref("db")));
	writer.put_setc(cast<WORD>(ref("dw")));
	writer.put_setc(cast<DWORD>(ref("dd")));
	writer.put_ret();

	writer.label("sanity");
	writer.put_mov(EAX, 0);
	writer.put_ret();

	writer.label("read_db");
	writer.put_mov(EAX, 0);
	writer.put_mov(AL, cast<BYTE>(ref("db")));
	writer.put_ret();

	writer.label("read_dw");
	writer.put_mov(EAX, 0);
	writer.put_mov(AX, cast<WORD>(ref("dw")));
	writer.put_ret();

	writer.label("read_dd");
	writer.put_mov(EAX, ref("dd"));
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_i32("main"), 1);
	CHECK(buffer.call_i32("sanity"), 0);
	CHECK(buffer.call_i32("read_db"), 1);
	CHECK(buffer.call_i32("read_dw"), 1);
	CHECK(buffer.call_i32("read_dd"), 1);

}

TEST (writer_exec_int_0x80) {

	BufferWriter writer;

	writer.put_mov(EAX, 20); // sys_getpid
	writer.put_int(0x80);
	writer.put_ret();

	const uint32_t pid = writer.bake().call_u32();
	CHECK(pid, getpid());

}

TEST (writer_exec_long_back_jmp) {

	BufferWriter writer;

	writer.label("start");
	writer.put_mov(EAX, 1);
	writer.put_ret();

	writer.label("main");
	writer.put_mov(EAX, 0);
	for (int i = 0; i < 255; i ++) {
		writer.put_nop();
	}

	writer.put_jmp("start");

	CHECK(writer.bake().call_u32("main"), 1);

}

TEST (writer_exec_imul_short) {

	BufferWriter writer;

	writer.put_mov(EAX, -3);
	writer.put_mov(EDX, 4);
	writer.put_imul(EAX, EDX);
	writer.put_ret();

	CHECK(writer.bake().call_u32(), -12);

}

TEST (writer_exec_memory_xchg) {

	BufferWriter writer;

	writer.label("foo").put_dword(123);
	writer.label("bar").put_byte(100);

	writer.label("main");
	writer.put_mov(EAX, 33);
	writer.put_mov(BL, 99);
	writer.put_xchg(EAX, ref("foo"));
	writer.put_xchg(cast<BYTE>(ref("bar")), BL);
	writer.put_ret();

	writer.label("get_foo");
	writer.put_mov(EAX, ref("foo"));
	writer.put_ret();

	writer.label("get_bar");
	writer.put_xor(EAX, EAX);
	writer.put_mov(AL, cast<BYTE>(ref("bar")));
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();

	CHECK(buffer.call_i32("main"), 123);
	CHECK(buffer.call_i32("get_foo"), 33);
	CHECK(buffer.call_i32("get_bar"), 99);

}

TEST (writer_exec_memory_push_pop) {

	BufferWriter writer;

	writer.label("foo").put_dword(123);
	writer.label("bar").put_word(100);
	writer.label("car").put_dword(0);

	writer.label("main");
	writer.put_xor(EBX, EBX);
	writer.put_mov(EAX, 0);
	writer.put_push(666);
	writer.put_push(ref(EAX + "foo"));
	writer.put_push(cast<WORD>(ref(EAX + "bar")));
	writer.put_pop(BX);
	writer.put_pop(EAX);
	writer.put_pop(ref("car"));
	writer.put_add(EAX, EBX);
	writer.put_ret();

	writer.label("get_car");
	writer.put_xor(EAX, EAX);
	writer.put_mov(EAX, ref("car"));
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();

	CHECK(buffer.call_i32("main"), 223);
	CHECK(buffer.call_i32("get_car"), 666);

}

TEST (writer_exec_memory_dec_inc) {

	BufferWriter writer;

	writer.label("ad").put_dword(150);
	writer.label("bd").put_word(100);
	writer.label("cd").put_byte(50);
	writer.label("ai").put_dword(150);
	writer.label("bi").put_word(100);
	writer.label("ci").put_byte(50);

	writer.label("main");
	writer.put_dec(ref("ad"));
	writer.put_dec(cast<WORD>(ref("bd")));
	writer.put_dec(cast<BYTE>(ref("cd")));
	writer.put_inc(ref("ai"));
	writer.put_inc(cast<WORD>(ref("bi")));
	writer.put_inc(cast<BYTE>(ref("ci")));
	writer.put_cmp(ref("ad"), 149);
	writer.put_jne("invalid");
	writer.put_cmp(cast<WORD>(ref("bd")), 99);
	writer.put_jne("invalid");
	writer.put_cmp(cast<BYTE>(ref("cd")), 49);
	writer.put_jne("invalid");
	writer.put_cmp(ref("ai"), 151);
	writer.put_jne("invalid");
	writer.put_cmp(cast<WORD>(ref("bi")), 101);
	writer.put_jne("invalid");
	writer.put_cmp(cast<BYTE>(ref("ci")), 51);
	writer.put_jne("invalid");
	writer.put_mov(EAX, 1);
	writer.put_ret();

	writer.label("invalid");
	writer.put_mov(EAX, 0);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();

	CHECK(buffer.call_i32("main"), 1);

}

TEST (writer_exec_memory_neg) {

	BufferWriter writer;

	writer.label("a").put_dword(150);
	writer.label("b").put_word(-100);
	writer.label("c").put_byte(50);

	writer.label("main");
	writer.put_neg(ref("a"));
	writer.put_neg(cast<WORD>(ref("b")));
	writer.put_neg(cast<BYTE>(ref("c")));
	writer.put_cmp(ref("a"), -150);
	writer.put_jne("invalid");
	writer.put_cmp(cast<WORD>(ref("b")), 100);
	writer.put_jne("invalid");
	writer.put_cmp(cast<BYTE>(ref("c")), -50);
	writer.put_jne("invalid");
	writer.put_mov(EAX, 1);
	writer.put_ret();

	writer.label("invalid");
	writer.put_mov(EAX, 0);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();

	CHECK(buffer.call_i32("main"), 1);

}

TEST (writer_exec_memory_not) {

	BufferWriter writer;

	writer.label("a").put_dword(12345);
	writer.label("b").put_word(178);
	writer.label("c").put_byte(34);

	writer.label("main");
	writer.put_not(ref("a"));
	writer.put_not(cast<WORD>(ref("b")));
	writer.put_not(cast<BYTE>(ref("c")));
	writer.put_cmp(ref("a"), ~12345);
	writer.put_jne("invalid");
	writer.put_cmp(cast<WORD>(ref("b")), ~178);
	writer.put_jne("invalid");
	writer.put_cmp(cast<BYTE>(ref("c")), ~34);
	writer.put_jne("invalid");
	writer.put_mov(EAX, 1);
	writer.put_ret();

	writer.label("invalid");
	writer.put_mov(EAX, 0);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();

	CHECK(buffer.call_i32("main"), 1);

}

TEST (writer_exec_memory_mul) {

	BufferWriter writer;

	writer.label("a").put_byte(17);
	writer.label("r").put_byte({2, 0, 0});

	writer.label("main");
	writer.put_mov(EAX, 3);
	writer.put_mul(ref("a")); // intentional dword ptr
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	uint32_t eax = buffer.call_i32("main");

	ASSERT(eax > 3*17);

}

TEST (writer_exec_memory_shift) {

	BufferWriter writer;

	writer.label("a").put_dword(0x0571BACD);

	writer.label("main");
	writer.put_xor(EAX, EAX);
	writer.put_shl(cast<DWORD>(ref("a")), 1);
	writer.put_mov(EAX, cast<DWORD>(ref("a")));
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_i32("main"), 0x0571BACD<<1);

}

TEST (writer_exec_xlat) {

	BufferWriter writer;
	writer.label("table");
	writer.put_byte(0); // 0 -> 0
	writer.put_byte(7); // 1 -> 7
	writer.put_byte(6); // 2 -> 6
	writer.put_byte(1); // 3 -> 1
	writer.put_byte(5); // 4 -> 5
	writer.put_byte(4); // 5 -> 4
	writer.put_byte(3); // 6 -> 3
	writer.put_byte(0); // 7 -> 0

	writer.label("main");
	writer.put_mov(EBX, "table");
	writer.put_mov(AL, 2);
	writer.put_xlat(); // AL := EBX[1] -> 6
	writer.put_xlat(); // AL := EBX[6] -> 3
	writer.put_xlat(); // AL := EBX[3] -> 1
	writer.put_xlat(); // AL := EBX[1] -> 7
	writer.put_cmp(AL, 7);
	writer.put_jne("invalid");

	writer.put_mov(AL, 7);
	writer.put_xlat(); // AL := EBX[7] -> 0
	writer.put_xlat(); // AL := EBX[0] -> 0
	writer.put_xlat(); // AL := EBX[0] -> 0
	writer.put_xlat(); // AL := EBX[0] -> 0
	writer.put_cmp(AL, 0);
	writer.put_jne("invalid");

	writer.put_mov(AL, 4);
	writer.put_xlat(); // AL := EBX[4] -> 5
	writer.put_xlat(); // AL := EBX[5] -> 4
	writer.put_xlat(); // AL := EBX[4] -> 5
	writer.put_xlat(); // AL := EBX[5] -> 4
	writer.put_cmp(AL, 4);
	writer.put_jne("invalid");
	writer.put_mov(EAX, 1);
	writer.put_ret();

	writer.label("invalid");
	writer.put_mov(EAX, 0);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_i32("main"), 1);

}

TEST (writer_exec_scf) {

	BufferWriter writer;

	writer.label("main");
	writer.put_scf(1);
	writer.put_jnc("invalid");
	writer.put_scf(0);
	writer.put_jc("invalid");
	writer.put_mov(EAX, 1);
	writer.put_ret();

	writer.label("invalid");
	writer.put_mov(EAX, 0);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_i32("main"), 1);

}

TEST (writer_exec_test) {

	BufferWriter writer;

	writer.label("foo");
	writer.put_dword(0x10000);

	writer.label("main");
	writer.put_test(ref("foo"), 0x10000);
	writer.put_je("invalid");
	writer.put_test(ref("foo"), 0x20000);
	writer.put_jne("invalid");
	writer.put_mov(EDX, 0x10000);
	writer.put_test(EDX, ref("foo"));
	writer.put_je("invalid");
	writer.put_test(ref("foo"), EDX);
	writer.put_je("invalid");
	writer.put_mov(EAX, 1);
	writer.put_ret();

	writer.label("invalid");
	writer.put_mov(EAX, 0);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_i32("main"), 1);

}

TEST (writer_exec_test_eax_eax) {

	BufferWriter writer;

	writer.label("main_1");
	writer.put_mov(EDX, 0);
	writer.put_test(EDX);
	writer.put_jnz("invalid");
	writer.put_mov(EAX, 1);
	writer.put_ret();

	writer.label("main_2");
	writer.put_mov(EDX, 1);
	writer.put_test(EDX);
	writer.put_jz("invalid");
	writer.put_mov(EAX, 1);
	writer.put_ret();

	writer.label("main_3");
	writer.put_mov(EDX, 7);
	writer.put_test(EDX);
	writer.put_jz("invalid");
	writer.put_mov(EAX, 1);
	writer.put_ret();

	writer.label("invalid");
	writer.put_mov(EAX, 0);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_i32("main_1"), 1);
	CHECK(buffer.call_i32("main_2"), 1);
	CHECK(buffer.call_i32("main_3"), 1);

}

TEST (writer_exec_shld_shrd) {

	BufferWriter writer;

	writer.label("shld");
	writer.put_xor(EAX, EAX);
	writer.put_xor(EDX, EDX);
	writer.put_mov(AX, 0b01001000'01001110);
	writer.put_mov(DX, 0b10100001'00010001);
	writer.put_mov(CL, 4);
	writer.put_shld(AX, DX, CL);

	writer.put_shl(EDX, 4*4);
	writer.put_shld(ref("var"), EDX, 4*4);
	writer.put_ret();

	writer.label("shrd");
	writer.put_xor(EAX, EAX);
	writer.put_mov(AX, 0b01001000'01001110);
	writer.put_mov(DX, 0b10100001'00010001);
	writer.put_shrd(AX, DX, 4);
	writer.put_ret();

	writer.label("check");
	writer.put_mov(EAX, ref("var"));
	writer.put_ret();

	writer.label("var").put_dword(0b11110111'00110001);

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u32("shld"), 0b10000100'11101010);
	CHECK(buffer.call_u32("shrd"), 0b00010100'10000100);
	CHECK(buffer.call_u32("check"), 0b11110111'00110001'10100001'00010001);

}

TEST (writer_exec_lods) {

	BufferWriter writer;

	writer.label("main");
	writer.put_mov(EAX, 0);
	writer.put_mov(ESI, "src");
	writer.put_lodsw();
	writer.put_lodsb();
	writer.put_ret();

	writer.label("src").put_ascii("Ugus!");

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u32("main"), (('g' << 8) | 'u'));

}

TEST (writer_exec_stos) {

	BufferWriter writer;

	writer.label("main");
	writer.put_mov(EDI, "dst");
	writer.put_mov(AL, 'H');
	writer.put_stosb();
	writer.put_mov(AL, 'e');
	writer.put_stosb();
	writer.put_mov(AX, (('l' << 8) + 'l'));
	writer.put_stosw();
	writer.put_mov(AL, 'o');
	writer.put_stosb();
	writer.put_mov(AL, '!');
	writer.put_stosb();
	writer.put_mov(AL, '\0');
	writer.put_stosb();
	writer.put_ret();

	writer.label("dst").put_space(16);

	ExecutableBuffer buffer = writer.bake();
	buffer.call_u32("main");

	ASSERT(strcmp((char*) buffer.address("dst"), "Hello!") == 0);

}

TEST (writer_exec_movs) {

	BufferWriter writer;

	writer.label("main");
	writer.put_mov(ESI, "src");
	writer.put_mov(EDI, "dst");
	writer.put_mov(ECX, 10);
	writer.put_rep().put_movsb();
	writer.put_ret();

	writer.label("src").put_ascii("123456789ABCDEF"); // null byte included
	writer.label("dst").put_space(16);

	ExecutableBuffer buffer = writer.bake();
	buffer.call_u32("main");

	ASSERT(strcmp((char*) buffer.address("dst"), "123456789A") == 0);

}

TEST (writer_exec_cmps) {

	BufferWriter writer;

	writer.label("main_1");
	writer.put_mov(ESI, "src");
	writer.put_mov(EDI, "dst_1");
	writer.put_mov(ECX, 10);
	writer.put_repe().put_cmpsb();
	writer.put_jne("invalid");
	writer.put_mov(EAX, 1);
	writer.put_ret();

	writer.label("main_2");
	writer.put_mov(ESI, "src");
	writer.put_mov(EDI, "dst_2");
	writer.put_mov(ECX, 10);
	writer.put_repe().put_cmpsb();
	writer.put_je("invalid");
	writer.put_mov(EAX, 1);
	writer.put_ret();

	writer.label("invalid");
	writer.put_mov(EAX, 0);
	writer.put_ret();

	writer.label("src"  ).put_ascii("123456789ABCDEF");
	writer.label("dst_1").put_ascii("123456789AB---F");
	writer.label("dst_2").put_ascii("123456-89ABCDEF");

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u32("main_1"), 1);
	CHECK(buffer.call_u32("main_2"), 1);

}

TEST (writer_exec_scas) {

	BufferWriter writer;

	writer.label("main");
	writer.put_mov(EDI, "src");
	writer.put_mov(ECX, 16);
	writer.put_mov(AL, 'D');
	writer.put_repne().put_scasb();
	writer.put_sub(EDI, "src");
	writer.put_mov(EAX, EDI);
	writer.put_ret();

	writer.label("src").put_ascii("123456789ABCDEF");

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u32("main"), 13);

}

TEST (writer_exec_tricky_encodings) {

	BufferWriter writer;

	writer.label("sanity");
	writer.put_push(EBP);
	writer.put_lea(EAX, 1);
	writer.put_pop(EBP);
	writer.put_ret();

	// lea eax, 1234h
	// special case: no base nor index
	writer.label("test_1");
	writer.put_lea(EAX, 0x1234);
	writer.put_ret();

	// mov eax, esp
	// special case: ESP used in r/m
	writer.label("test_2");
	writer.put_push(EBP);
	writer.put_mov(EBP, ESP);
	writer.put_mov(ESP, 16492);
	writer.put_mov(EAX, ESP); // here
	writer.put_mov(ESP, EBP);
	writer.put_pop(EBP);
	writer.put_ret();

	// lea eax, ebp + eax
	// special case: EBP in SIB with no offset
	writer.label("test_3");
	writer.put_push(EBP);
	writer.put_mov(EBP, 12);
	writer.put_mov(EDX, 56);
	writer.put_lea(EAX, EBP + EDX);
	writer.put_pop(EBP);
	writer.put_ret();

	// lea eax, edx * 2
	// special case: no base in SIB
	writer.label("test_4");
	writer.put_mov(EDX, 56);
	writer.put_lea(EAX, EDX * 2);
	writer.put_ret();

	// lea eax, ebp + 1234h
	// special case: no index in SIB
	writer.label("test_5");
	writer.put_push(EBP);
	writer.put_mov(EBP, 111);
	writer.put_lea(EAX, EBP + 333);
	writer.put_pop(EBP);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_i32("sanity"), 1);
	CHECK(buffer.call_i32("test_1"), 0x1234);
	CHECK(buffer.call_i32("test_2"), 16492);
	CHECK(buffer.call_i32("test_3"), 12 + 56);
	CHECK(buffer.call_i32("test_4"), 56 * 2);
	CHECK(buffer.call_i32("test_5"), 444);

}

TEST (writer_exec_pushf_popf) {

	BufferWriter writer;

	writer.put_xor(EAX, EAX);
	writer.put_stc();
	writer.put_pushf();
	writer.put_clc();
	writer.put_popf();
	writer.put_setc(EAX);
	writer.put_ret();

	CHECK(writer.bake().call_i32(), 1);

}

TEST (writer_exec_retx) {

	BufferWriter writer;

	writer.label("stdcall");
	writer.put_mov(EAX, ref(ESP + 4));
	writer.put_mov(EDX, ref(ESP + 8));
	writer.put_sub(ESP, 4);
	writer.put_mov(ref(ESP), EAX);
	writer.put_add(ref(ESP), EDX);
	writer.put_mov(EAX, ref(ESP));
	writer.put_add(ESP, 4);
	writer.put_ret(8);

	writer.label("main");
	writer.put_mov(ECX, ESP);
	writer.put_push(21);
	writer.put_push(4300);
	writer.put_call("stdcall");
	writer.put_sub(ECX, ESP);
	writer.put_add(EAX, ECX);
	writer.put_ret();

	CHECK(writer.bake().call_i32("main"), 4321);

}

TEST (writer_exec_enter_leave) {

	BufferWriter writer;

	writer.label("stdcall");
	writer.put_enter(4, 0);
	writer.put_scf(0);
	writer.put_mov(ref(EBP - 4), 1230);
	writer.put_adc(ref(EBP - 4), 4);
	writer.put_mov(EAX, ref(EBP - 4));
	writer.put_mov(ESP, 21);
	writer.put_leave();
	writer.put_ret();

	writer.label("main");
	writer.put_mov(EBP, 0);
	writer.put_mov(ECX, ESP);
	writer.put_call("stdcall");
	writer.put_sub(ECX, ESP);
	writer.put_add(EAX, ECX);
	writer.put_add(EAX, EBP);
	writer.put_ret();

	CHECK(writer.bake().call_i32("main"), 1234);

}

TEST (writer_exec_bsf_bsr) {

	BufferWriter writer;

	//                                        ,-> 21    ,-> 13
	//                                        |         |
	writer.label("a").put_dword(0b0000'0000'0010'0000'0010'0000'0000'0000);

	//                                  ,-> 9
	//                                  |
	writer.label("b").put_word(0b0000'0010'0000'0000);

	writer.label("bsf_a");
	writer.put_bsf(EAX, ref("a"));
	writer.put_ret();

	writer.label("bsr_a");
	writer.put_xor(EDX, EDX);
	writer.put_bsr(EAX, ref(EDX + "a"));
	writer.put_ret();

	writer.label("bsf_b");
	writer.put_xor(EDX, EDX);
	writer.put_mov(EAX, 0x80000000);
	writer.put_bsf(AX, ref(EDX + "b"));
	writer.put_ret();

	writer.label("bsr_b");
	writer.put_mov(EAX, 0x80000000);
	writer.put_bsr(AX, ref("b"));
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u32("bsf_a"), 13);
	CHECK(buffer.call_u32("bsr_a"), 21);
	CHECK(buffer.call_u32("bsf_b"), 0x80000009);
	CHECK(buffer.call_u32("bsr_b"), 0x80000009);

}

TEST (writer_fail_redefinition) {

	BufferWriter writer;

	writer.label("a").put_byte(1);
	writer.label("b").put_byte(2);
	writer.label("main");
	writer.put_ret();

	EXPECT(std::runtime_error, {
		writer.label("main");
	});

	EXPECT(std::runtime_error, {
		writer.label("a");
	});

	EXPECT(std::runtime_error, {
		writer.label("b");
	});

}

TEST (writer_fail_invalid_reg_size) {

	BufferWriter writer;

	writer.put_movsx(AX, DL);  // valid
	writer.put_movsx(EAX, BH); // valid
	writer.put_movsx(EDX, AX); // valid

	EXPECT(std::runtime_error, {
		writer.put_mov(EAX, AX);
	});

	EXPECT(std::runtime_error, {
		writer.put_mov(AX, EAX);
	});

	EXPECT(std::runtime_error, {
		writer.put_movsx(AX, AX);
	});

	EXPECT(std::runtime_error, {
		writer.put_movsx(AL, AX);
	});

	EXPECT(std::runtime_error, {
		writer.put_movsx(BH, EDX);
	});

	EXPECT(std::runtime_error, {
		writer.put_xchg(cast<BYTE>(ref("bar")), EBX);
	});

	EXPECT(std::runtime_error, {
		writer.put_xchg(cast<WORD>(ref("bar")), EBX);
	});

	EXPECT(std::runtime_error, {
		writer.put_xchg(cast<BYTE>(ref("bar")), BX);
	});

}

TEST (writer_fail_invalid_mem_size) {

	BufferWriter writer;

	writer.label("a");
	writer.put_dword(0);

	writer.put_mov(AL, cast<BYTE>(ref("a"))); // valid
	writer.put_mov(AL, 0xFFFFFFFF);           // stupid, but valid

	EXPECT(std::runtime_error, {
		writer.put_fst(cast<TWORD>(ref("a")));
	});

	EXPECT(std::runtime_error, {
		writer.put_fst(cast<WORD>(ref("a")));
	});

	EXPECT(std::runtime_error, {
		writer.put_fst(cast<BYTE>(ref("a")));
	});

	EXPECT(std::runtime_error, {
		writer.put_mov(EAX, cast<BYTE>(ref("a")));
	});

}

TEST (writer_fail_undefined_label) {

	BufferWriter writer;
	writer.put_mov(EAX, ref("hamburger"));

	EXPECT(std::runtime_error, {
		writer.bake();
	});

}

TEST (writer_elf_execve) {

	using namespace asmio::elf;

	BufferWriter writer;

	writer.label("text").put_ascii("Hello World!\n");

	writer.label("strlen");
	writer.put_mov(ECX, EAX);
	writer.put_dec(EAX);
	writer.label("l_strlen_next");
	writer.put_inc(EAX);
	writer.put_cmp(cast<BYTE>(ref(EAX)), 0);
	writer.put_jne("l_strlen_next");
	writer.put_sub(EAX, ECX);
	writer.put_ret();

	writer.label("_start");
	writer.put_lea(EAX, "text");
	writer.put_call("strlen");
	writer.put_mov(EBX, EAX); // exit code
	writer.put_mov(EAX, 1); // sys_exit
	writer.put_int(0x80);
	writer.put_ret();

	ElfBuffer file = writer.bake_elf(nullptr);

	int status;
	RunResult result = file.execute("memfd-elf-1", &status);

	CHECK(result, RunResult::SUCCESS);
	CHECK(status, 13);

}

TEST (tasml_tokenize) {

	std::string code = R"(
		text:
			byte "Hello!", 0

		strlen:
			mov ecx, /* inline comments! */ eax
			dec eax

			l_strlen_next:
				inc eax
				cmp byte [eax], 0
			jne @l_strlen_next

			sub eax, ecx
			ret

		_start:
			lea eax, @text
			call @strlen
			nop; nop; ret // multi-statements
	)";

	BufferWriter writer;
	tasml::ErrorHandler reporter {"<string>", true};

	std::vector<tasml::Token> tokens = tasml::tokenize(reporter, code);

	if (!reporter.ok()) {
		reporter.dump();
		FAIL("Tokenizer error!");
	}

	tasml::TokenStream stream {tokens};
	parseBlock(reporter, writer, stream);

	if (!reporter.ok()) {
		reporter.dump();
		FAIL("Parser error!");
	}

	CHECK(writer.bake().call_i32("_start"), 6);

}

BEGIN(VSTL_MODE_LENIENT)