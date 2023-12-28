
#define DEBUG_MODE false

#include "lib/vstl.hpp"
#include "writer.hpp"

using namespace asmio::x86;

TEST(writer_exec_mov_ret_nop) {

	BufferWriter writer;

	writer.put_mov(EAX, 5);
	writer.put_nop();
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	uint32_t eax = buffer.call_u32();

	CHECK(eax, 5);

}

TEST(writer_exec_rol_inc_neg_sar) {

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

TEST(writer_simple_xchg) {

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
	CHECK(buffer.call_u32("main"), 14 + buffer.get_address());

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

TEST (writer_exec_fnop_finit_fld1) {

	BufferWriter writer;

	writer.put_fnop();
	writer.put_finit();
	writer.put_fld1();
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32(), 1.0f);

}

TEST (writer_exec_fmul_fimul_fmulp) {

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

TEST(writer_exec_f2xm1_fabs_fchs) {

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

TEST (writer_exec_fadd_fiadd_faddp) {

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

TEST (writer_exec_fcom_fstsw_fcomp_fcmove_fcmovb) {

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

TEST (writer_exec_fcompp) {

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

TEST (writer_exec_fcomi_fcomip) {

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

TEST (writer_exec_fcos) {

	BufferWriter writer;

	writer.label("main");
	writer.put_finit();
	writer.put_fldpi();        // fpu stack: [PI]
	writer.put_fcos();         // fpu stack: [cos(PI)]
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_f32("main"), -1);

}

TEST (writer_exec_fdiv_fdivp) {

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

TEST (writer_exec_fdivr_fdivrp) {

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

TEST (writer_exec_ficom_ficomp) {

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

BEGIN(VSTL_MODE_LENIENT)