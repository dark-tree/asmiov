
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

TEST (writer_check_push) {

	BufferWriter writer;

	// 16 bit
	writer.put_push(AX);
	writer.put_push(BX);
	writer.put_push(BP);
	writer.put_push(R9W);
	writer.put_push(ref<WORD>(RAX));

	// 64 bit
	writer.put_push(RAX);
	writer.put_push(RBX);
	writer.put_push(RBP);
	writer.put_push(R11);
	writer.put_push(R13);
	writer.put_push(R15);
	writer.put_push(ref<QWORD>(RAX));

	// 8 bit
	EXPECT_ANY({ writer.put_push(AL); });
	EXPECT_ANY({ writer.put_push(AH); });
	EXPECT_ANY({ writer.put_push(SPL); });
	EXPECT_ANY({ writer.put_push(ref<BYTE>(RAX)); });

	// 32 bit
	EXPECT_ANY({ writer.put_push(EAX); });
	EXPECT_ANY({ writer.put_push(R10D); });
	EXPECT_ANY({ writer.put_push(R15D); });
	EXPECT_ANY({ writer.put_push(ref<DWORD>(RAX)); });

	// too large imm
	EXPECT_ANY({ writer.put_push(0xffffffffff); });

}

TEST (writer_check_mov_sizing) {

	BufferWriter writer;

	EXPECT_ANY({ writer.put_mov(EAX, AX); });
	EXPECT_ANY({ writer.put_mov(ref(RAX), ref(RAX)); });

}

TEST (writer_check_mov_address_size) {

	BufferWriter writer;

	writer.put_mov(AL, ref(RDX)); // 8a 02
	writer.put_mov(AL, ref(EDX)); // 67 8a 02

	EXPECT_ANY({ writer.put_mov(AL, ref(DX)); });
	EXPECT_ANY({ writer.put_mov(AL, ref(SIL)); });

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.size(), 5);

}

TEST (writer_check_high_byte_register) {

	BufferWriter writer;

	// ok, legacy low/high registers
	writer.put_mov(AH, DH);
	writer.put_mov(BH, AL);

	// ok, only extended low registers
	writer.put_mov(SIL, DIL);
	writer.put_mov(BPL, SIL);

	// ok, legacy/extended low registers
	writer.put_mov(SIL, AL);
	writer.put_mov(DL, BPL);

	// error, extended low and legacy high
	EXPECT_ANY({ writer.put_mov(SIL, AH); });
	EXPECT_ANY({ writer.put_mov(BH, BPL); });

}

TEST (writer_check_stack_index_register) {

	BufferWriter writer;

	// ok, sanity checks
	writer.put_mov(EAX, ESP);
	writer.put_mov(RAX, RSP);
	writer.put_mov(EAX, ref(RSP));
	writer.put_mov(RAX, ref(RSP));
	writer.put_mov(EAX, ref(RSP + 4));
	writer.put_mov(RAX, ref(RSP + 4));

	// ok, ESP/RSP is the base here
	writer.put_mov(EAX, ref(RSP + RAX));
	writer.put_mov(EAX, ref(ESP + EAX));

	// error, ESP/RSP can't be used as index
	EXPECT_ANY({ writer.put_mov(EAX, ref(RAX + RSP * 2 + 4)); });
	EXPECT_ANY({ writer.put_mov(EAX, ref(RAX + RSP * 2)); });
	EXPECT_ANY({ writer.put_mov(EAX, ref(EAX + ESP * 2 + 4)); });
	EXPECT_ANY({ writer.put_mov(EAX, ref(EAX + ESP * 2)); });

	// this can be made correct by swapping registers
	EXPECT_ANY({ writer.put_mov(EAX, ref(RAX + RSP)); });
	EXPECT_ANY({ writer.put_mov(EAX, ref(EAX + ESP)); });

}

TEST (writer_check_lea_sizing) {

	BufferWriter writer;

	// ok
	writer.put_lea(RAX, 0);

	// ok, use address size prefix
	writer.put_lea(EAX, 0);

	// error
	EXPECT_ANY({ writer.put_lea(AX, 0); });
	EXPECT_ANY({ writer.put_lea(AL, 0); });

}

TEST (writer_check_mixed_addressing) {

	BufferWriter writer;

	writer.put_mov(RAX, ref(EAX + EBX * 2 + 123));
	writer.put_mov(EAX, ref(RAX + RBX * 2 + 123));

	EXPECT_ANY({ writer.put_mov(RAX, ref(RAX + EBX * 2 + 123)); });
	EXPECT_ANY({ writer.put_mov(RAX, ref(EAX + RBX * 2 + 123)); });

}

TEST (writer_check_st_as_generic) {

	BufferWriter writer;

	EXPECT_ANY({ writer.put_inc(ST); });
	EXPECT_ANY({ writer.put_mov(RAX, ST); });
	EXPECT_ANY({ writer.put_lea(EAX, ST + 6); });


}

TEST (legacy_rbp_protection) {

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

TEST (writer_exec_mov_long) {

	BufferWriter writer;

	writer.put_mov(RAX, 0x1000000000000000);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	uint64_t eax = buffer.call_u64();

	CHECK(eax, 0x1000000000000000);

}

TEST (writer_exec_mov_long_simple) {

	BufferWriter writer;

	writer.put_mov(RDX, 0x2000000000000000);
	writer.put_mov(RAX, RDX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	uint64_t eax = buffer.call_u64();

	CHECK(eax, 0x2000000000000000);

}

TEST (writer_exec_mov_add_long_regmem) {

	BufferWriter writer;

	writer.label("L1");
	writer.put_qword(7);
	writer.put_qword(3);
	writer.put_qword(9);

	writer.label("start");
	writer.put_mov(RAX, 0);
	writer.put_mov(R13, 42);
	writer.put_mov(R12, 11);
	writer.put_mov(R14, R12);

	writer.put_mov(R15, 1);
	writer.put_add(R13, R14);
	writer.put_add(R13, ref(R15 * 8 + "L1"));

	writer.put_mov(RAX, R13);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	uint64_t rax = buffer.call_u64("start");
	CHECK(rax, 56);

}

TEST (writer_exec_mov_add_extended) {

	BufferWriter writer;

	writer.label("L1"); // => 7
	writer.put_xor(RAX, RAX);
	writer.put_mov(R8L, 3);
	writer.put_mov(R9L, 4);
	writer.put_add(R8L, R9L);
	writer.put_mov(AL, R8L);
	writer.put_ret();

	writer.label("L2"); // => 13
	writer.put_xor(RAX, RAX);
	writer.put_mov(SIL, 11);
	writer.put_mov(DIL, 2);
	writer.put_add(SIL, DIL);
	writer.put_mov(AL, SIL);
	writer.put_ret();

	writer.label("L3"); // => 1700
	writer.put_xor(RAX, RAX);
	writer.put_mov(R12W, 900);
	writer.put_mov(R13W, 800);
	writer.put_add(R12W, R13W);
	writer.put_mov(AX, R12W);
	writer.put_ret();

	writer.label("L4"); // => 170000
	writer.put_xor(RAX, RAX);
	writer.put_mov(R12D, 90000);
	writer.put_mov(R13D, 80000);
	writer.put_add(R12D, R13D);
	writer.put_mov(EAX, R12D);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u64("L1"), 7);
	CHECK(buffer.call_u64("L2"), 13);
	CHECK(buffer.call_u64("L3"), 1700);
	CHECK(buffer.call_u64("L4"), 170000);

}

TEST (writer_exec_mem_moves) {

	BufferWriter writer;

	writer.label("a").put_dword();
	writer.label("b").put_dword();
	writer.label("c").put_dword(0x300);

	writer.label("main");

	// test 1
	writer.put_mov(EAX, 1200);
	writer.put_mov(ref("a"), EAX);
	writer.put_mov(EDX, ref("a"));
	writer.put_cmp(EDX, 1200);
	writer.put_jne("fail");

	// test 2
	writer.put_mov(ref<DWORD>("b"), 121);
	writer.put_mov(EDX, ref("b"));
	writer.put_cmp(EDX, 121);
	writer.put_jne("fail");

	// test 3
	writer.put_mov(EDX, ref("c"));
	writer.put_cmp(EDX, 0x300);
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

	writer.put_mov(RAX, 9);
	writer.put_push(RAX);
	writer.put_mov(RAX, 7);
	writer.put_pop(RCX);
	writer.put_mov(RAX, RCX);
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

TEST(writer_exec_add_long) {

	BufferWriter writer;

	writer.label("data");
	writer.put_byte(0x16);
	writer.put_byte(0x49);
	writer.put_word(0x2137);
	writer.put_dword(0x1111'2222);
	writer.put_qword(0x1'0000'0000);

	writer.label("code");
	writer.put_mov(RAX, 0);
	writer.put_mov(RDX, 0);
	writer.put_mov(R15L, ref(RDX + "data")); // => 0x16
	writer.put_inc(RDX);

	writer.put_mov(R14L, ref(RDX + "data")); // => 0x49
	writer.put_inc(RDX);

	writer.put_mov(R13W, ref(RDX + "data")); // => 0x2137
	writer.put_add(RDX, 2);

	writer.put_mov(R12D, ref(RDX + "data")); // => 0x1111'2222
	writer.put_add(RDX, 4);

	writer.put_mov(R11, ref(RDX + "data")); // => 0x1'0000'0000

	writer.put_add(R15L, R14L);
	writer.put_add(R15W, R13W);
	writer.put_add(R15D, R12D);
	writer.put_add(R15, R11);

	writer.put_mov(RAX, R15);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u64("code"), (0x16 + 0x49 + 0x2137 + 0x1111'2222 + 0x1'0000'0000));

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

TEST(writer_exec_bts_long) {

	BufferWriter writer;

	writer.put_mov(R13, 0);

	writer.put_bts(R13, 63);
	writer.put_bts(R13, 40);
	writer.put_bts(R13, 9);

	writer.put_mov(RAX, R13);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u64(), 0x8000'0100'0000'0200);

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
	writer.put_add(EAX, ref(RSP + 8));
	writer.put_ret();

	writer.label("main");
	writer.put_mov(EAX, 0);

	// add 20 to EAX
	writer.put_push(20);
	writer.put_call("add");
	writer.put_pop();

	// add 12 to EAX
	writer.put_push(12);
	writer.put_call("add");
	writer.put_pop();

	// add 10 to EAX
	writer.put_push(10);
	writer.put_call("add");
	writer.put_pop();

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
	writer.put_fld(ref<DWORD>("a"));       // fpu stack: [+6.0]
	writer.put_fld(ref<DWORD>("b"));       // fpu stack: [+0.5, +6.0]
	writer.put_fmul(ST + 0, ST + 1);       // fpu stack: [0.5*6.0, +6.0]
	writer.put_fimul(ref<DWORD>("c"));     // fpu stack: [0.5*6.0*4.0, +6.0]
	writer.put_fld(ref<QWORD>("d"));       // fpu stack: [0.25, 0.5*6.0*4.0, +6.0]
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
	writer.put_fld(ref<DWORD>("b")); // fpu stack: [+5.0f]
	writer.put_fld(ref<DWORD>("a")); // fpu stack: [+1.0, +5.0f]
	writer.put_fchs();               // fpu stack: [-1.0, +5.0f]
	writer.put_f2xm1();              // fpu stack: [2^(-1.0)-1, +5.0f]
	writer.put_fmulp(ST + 1);        // fpu stack: [(2^(-1.0)-1)*5.0]
	writer.put_fabs();               // fpu stack: [|(2^(-1.0)-1)*5.0|]
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
	writer.put_fld(ref<DWORD>("a"));     // fpu stack: [+6.0]
	writer.put_fld(ref<DWORD>("b"));     // fpu stack: [+0.5, +6.0]
	writer.put_fadd(ST + 0, ST + 1);     // fpu stack: [0.5+6.0, +6.0]
	writer.put_fiadd(ref<DWORD>("c"));   // fpu stack: [0.5+6.0+4.0, +6.0]
	writer.put_fld(ref<QWORD>("d"));     // fpu stack: [0.25, 0.5+6.0+4.0, +6.0]
	writer.put_faddp(ST + 1);            // fpu stack: [0.5+6.0+4.0+0.25, +6.0]
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
	writer.put_fld(ref<DWORD>("a"));  // fpu stack: [+6.0]
	writer.put_fld(ref<DWORD>("b"));  // fpu stack: [+0.5, +6.0]
	writer.put_fld(ref<DWORD>("c"));  // fpu stack: [+4.0, +0.5, +6.0]
	writer.put_fcom(ref<DWORD>("a")); // fpu stack: [+4.0, +0.5, +6.0]
	writer.put_fstsw(AX);
	writer.put_sahf();
	writer.put_fcmovb(ST + 2);        // fpu stack: [+6.0, +0.5, +6.0]
	writer.put_fld(ref<DWORD>("c"));  // fpu stack: [+6.0, +6.0, +0.5, +6.0]
	writer.put_fcomp(ST + 2);         // fpu stack: [+6.0, +0.5, +6.0]
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
	writer.put_fld(ref<DWORD>("a"));  // fpu stack: [+6.0]
	writer.put_fld(ref<DWORD>("b"));  // fpu stack: [+0.5, +6.0]
	writer.put_fld(ref<DWORD>("c"));  // fpu stack: [+4.0, +0.5, +6.0]
	writer.put_fcompp();              // fpu stack: [+6.0]
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
	writer.put_fld(ref<DWORD>("b"));  // fpu stack: [+0.5]
	writer.put_fld(ref<DWORD>("c"));  // fpu stack: [+4.0, +0.5]
	writer.put_fcomi(ST + 1);         // fpu stack: [+4.0, +0.5]
	writer.put_jb("invalid");
	writer.put_je("invalid");
	writer.put_ja("main2");
	writer.put_jmp("invalid");

	writer.label("main2");
	writer.put_fld(ref<DWORD>("a"));  // fpu stack: [3.0f, +4.0, +0.5]
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
	writer.put_fld(ref<DWORD>("a")); // fpu stack: [+2.0]
	writer.put_fld(ref<DWORD>("b")); // fpu stack: [+8.0, +2.0]
	writer.put_fdiv(ST + 0, ST + 1); // fpu stack: [8.0/2.0, +2.0]
	writer.put_fld(ref<DWORD>("c")); // fpu stack: [+0.125, 8.0/2.0, +2.0]
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
	writer.put_fld(ref<DWORD>("a"));     // fpu stack: [+2.0]
	writer.put_fld(ref<DWORD>("a"));     // fpu stack: [+2.0, +2.0]
	writer.put_fdivr(ST + 0, ST + 1);    // fpu stack: [2.0/2.0, +2.0]
	writer.put_fld(ref<DWORD>("c"));     // fpu stack: [+12.0, 2.0/2.0, +2.0]
	writer.put_fdivrp(ST + 2);           // fpu stack: [2.0/2.0, 12.0/2.0]
	writer.put_faddp(ST + 1);            // fpu stack: [2.0/2.0+12.0/2.0]
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
	writer.put_fld(ref<DWORD>("af"));    // fpu stack: [2.0]
	writer.put_ficom(ref<DWORD>("ai"));  // fpu stack: [2.0]
	writer.put_fstsw(AX);
	writer.put_sahf();
	writer.put_jne("invalid");
	writer.put_fld(ref<DWORD>("bf"));    // fpu stack: [3.0, 2.0]
	writer.put_ficomp(ref<DWORD>("bi")); // fpu stack: [2.0]
	writer.put_fstsw(AX);
	writer.put_sahf();
	writer.put_jne("invalid");
	writer.put_ficomp(ref<DWORD>("bi")); // fpu stack: []
	writer.put_fstsw(AX);
	writer.put_sahf();
	writer.put_je("invalid");
	writer.put_fld1();                   // fpu stack: [1.0]
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
	writer.put_fild(ref<WORD>("a"));   // fpu stack: [1.0]
	writer.put_fild(ref<DWORD>("b"));  // fpu stack: [2.0, 1.0]
	writer.put_fild(ref<QWORD>("c"));  // fpu stack: [3.0, 2.0, 1.0]
	writer.put_faddp(ST + 1);          // fpu stack: [3.0+2.0, 1.0]
	writer.put_faddp(ST + 1);          // fpu stack: [3.0+2.0+1.0]
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
	writer.put_fld1();                 // fpu stack: [1.0]
	writer.put_fld1();                 // fpu stack: [1.0, 1.0]
	writer.put_fld1();                 // fpu stack: [1.0, 1.0, 1.0]
	writer.put_faddp(ST + 1);          // fpu stack: [2.0, 1.0]
	writer.put_fist(ref<DWORD>("a"));  // fpu stack: [2.0, 1.0]
	writer.put_fld1();                 // fpu stack: [1.0, 2.0, 1.0]
	writer.put_fld1();                 // fpu stack: [1.0, 1.0, 2.0, 1.0]
	writer.put_faddp(ST + 1);          // fpu stack: [2.0, 2.0, 1.0]
	writer.put_faddp(ST + 1);          // fpu stack: [4.0, 1.0]
	writer.put_fistp(ref<DWORD>("b")); // fpu stack: [1.0]
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
	writer.put_fld1();                  // fpu stack: [1.0]
	writer.put_fld(ref<DWORD>("a"));    // fpu stack: [3.9, 1.0]
	writer.put_fisttp(ref<DWORD>("b")); // fpu stack: [1.0]
	writer.put_ret();

	writer.label("main");
	writer.put_mov(EAX, ref<DWORD>("b"));
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
	writer.put_fld(ref<DWORD>("a")); // fpu stack: [16.0]
	writer.put_fsqrt();              // fpu stack: [4.0]
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

	writer.label("tword_c");
	writer.put_qword_f(0); // => 6
	// TODO invalid size here?

	writer.label("set_a");
	writer.put_finit();
	writer.put_fld1();                            // fpu stack: [1.0]
	writer.put_fld1();                            // fpu stack: [1.0, 1.0]
	writer.put_faddp(ST + 1);                     // fpu stack: [2.0]
	writer.put_fst(ref<DWORD>("dword_a"));        // fpu stack: [2.0]
	writer.put_ret();

	writer.label("set_b");
	writer.put_finit();
	writer.put_fld1();                            // fpu stack: [1.0]
	writer.put_fld1();                            // fpu stack: [1.0, 1.0]
	writer.put_fld(ref<DWORD>("dword_a"));        // fpu stack: [2.0, 1.0, 1.0]
	writer.put_faddp(ST + 1);                     // fpu stack: [3.0, 1.0]
	writer.put_fstp(ref<DWORD>("dword_b"));       // fpu stack: [1.0]
	writer.put_ret();

	writer.label("set_c");
	writer.put_finit();
	writer.put_fld0();                            // fpu stack: [0.0]
	writer.put_fld(ref<DWORD>("dword_a"));        // fpu stack: [2.0, 0.0]
	writer.put_fld(ref<DWORD>("dword_b"));        // fpu stack: [3.0, 2.0, 0.0]
	writer.put_fst(ST + 1);                       // fpu stack: [3.0, 3.0, 0.0]
	writer.put_faddp(ST + 1);                     // fpu stack: [6.0, 0.0]
	writer.put_fstp(ref<TWORD>("tword_c"));       // fpu stack: [0.0]
	writer.put_ret();

	writer.label("ctrl_sum");
	writer.put_finit();
	writer.put_fld(ref<DWORD>("dword_a"));        // fpu stack: [2.0]
	writer.put_fld(ref<DWORD>("dword_b"));        // fpu stack: [3.0, 2.0]
	writer.put_fld(ref<TWORD>("tword_c"));        // fpu stack: [6.0, 3.0, 2.0]
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
	writer.put_mov(ref<DWORD>("var"), 17); // 10001
	writer.put_dec(ref<DWORD>("var"));     // 10000

	writer.put_bt(ref<DWORD>("var"), 4);
	writer.put_jnc("invalid_1");

	writer.put_bt(ref<DWORD>("var"), 0);
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
	writer.put_xchg(ref("bar"), BL);
	writer.put_ret();

	writer.label("get_foo");
	writer.put_mov(EAX, ref("foo"));
	writer.put_ret();

	writer.label("get_bar");
	writer.put_xor(EAX, EAX);
	writer.put_mov(AL, ref("bar"));
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

	// just to align the disassembly
	writer.put_byte(0);

	writer.label("main");
	writer.put_xor(EBX, EBX);
	writer.put_mov(EAX, 0);
	writer.put_push(666);
	writer.put_push(ref<QWORD>(EAX + "foo"));
	writer.put_push(ref<WORD>(EAX + "bar"));
	writer.put_pop(BX);
	writer.put_pop(EAX);
	writer.put_pop(ref<QWORD>("car"));
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
	writer.put_dec(ref<DWORD>("ad"));
	writer.put_dec(ref<WORD>("bd"));
	writer.put_dec(ref<BYTE>("cd"));
	writer.put_inc(ref<DWORD>("ai"));
	writer.put_inc(ref<WORD>("bi"));
	writer.put_inc(ref<BYTE>("ci"));
	writer.put_cmp(ref<DWORD>("ad"), 149);
	writer.put_jne("invalid");
	writer.put_cmp(ref<WORD>("bd"), 99);
	writer.put_jne("invalid");
	writer.put_cmp(ref<BYTE>("cd"), 49);
	writer.put_jne("invalid");
	writer.put_cmp(ref<DWORD>("ai"), 151);
	writer.put_jne("invalid");
	writer.put_cmp(ref<WORD>("bi"), 101);
	writer.put_jne("invalid");
	writer.put_cmp(ref<BYTE>("ci"), 51);
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
	writer.put_neg(ref<DWORD>("a"));
	writer.put_neg(ref<WORD>("b"));
	writer.put_neg(ref<BYTE>("c"));
	writer.put_cmp(ref<DWORD>("a"), -150);
	writer.put_jne("invalid");
	writer.put_cmp(ref<WORD>("b"), 100);
	writer.put_jne("invalid");
	writer.put_cmp(ref<BYTE>("c"), -50);
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
	writer.put_not(ref<DWORD>("a"));
	writer.put_not(ref<WORD>("b"));
	writer.put_not(ref<BYTE>("c"));
	writer.put_cmp(ref<DWORD>("a"), ~12345);
	writer.put_jne("invalid");
	writer.put_cmp(ref<WORD>("b"), ~178);
	writer.put_jne("invalid");
	writer.put_cmp(ref<BYTE>("c"), ~34);
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
	writer.put_mul(ref<DWORD>("a")); // intentional dword ptr
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
	writer.put_test(ref<DWORD>("foo"), 0x10000);
	writer.put_je("invalid");
	writer.put_test(ref<DWORD>("foo"), 0x20000);
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
	writer.put_push(RBP);
	writer.put_lea(EAX, 1);
	writer.put_pop(RBP);
	writer.put_ret();

	// lea eax, 1234h
	// special case: no base nor index
	writer.label("test_1");
	writer.put_lea(EAX, 0x1234);
	writer.put_ret();

	// mov eax, esp
	// special case: ESP used in r/m
	writer.label("test_2");
	writer.put_push(RBP);
	writer.put_mov(RBP, RSP);
	writer.put_mov(ESP, 16492);
	writer.put_mov(EAX, ESP); // here
	writer.put_mov(RSP, RBP);
	writer.put_pop(RBP);
	writer.put_ret();

	// lea eax, ebp + eax
	// special case: EBP in SIB with no offset
	writer.label("test_3");
	writer.put_push(RBP);
	writer.put_mov(EBP, 12);
	writer.put_mov(EDX, 56);
	writer.put_lea(EAX, EBP + EDX); // here
	writer.put_pop(RBP);
	writer.put_ret();

	// lea eax, edx * 2
	// special case: no base in SIB
	writer.label("test_4");
	writer.put_mov(EDX, 56);
	writer.put_lea(EAX, EDX * 2); // here
	writer.put_ret();

	// lea eax, ebp + 1234h
	// special case: no index in SIB
	writer.label("test_5");
	writer.put_push(RBP);
	writer.put_mov(EBP, 111);
	writer.put_lea(EAX, EBP + 333); // here
	writer.put_pop(RBP);
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
	writer.put_mov(EAX, ref(RSP + 8));
	writer.put_mov(EDX, ref(RSP + 16));
	writer.put_sub(RSP, 8);
	writer.put_mov(ref(RSP), EAX);
	writer.put_add(ref(RSP), EDX);
	writer.put_mov(EAX, ref(RSP));
	writer.put_add(RSP, 8);
	writer.put_ret(16);

	writer.label("main");
	writer.put_mov(RCX, RSP);
	writer.put_push(21);
	writer.put_push(4300);
	writer.put_call("stdcall");
	writer.put_sub(RCX, RSP);
	writer.put_add(EAX, ECX);
	writer.put_ret();

	auto baked = writer.bake();
	CHECK(baked.call_i32("main"), 4321);

}

TEST (writer_exec_enter_leave) {

	BufferWriter writer;

	writer.label("stdcall");
	writer.put_enter(8, 0);
	writer.put_scf(0);
	writer.put_mov(ref<DWORD>(RBP - 8), 1230);
	writer.put_adc(ref<DWORD>(RBP - 8), 4);
	writer.put_mov(EAX, ref(RBP - 8));
	writer.put_mov(ESP, 21);
	writer.put_leave();
	writer.put_ret();

	writer.label("main");
	writer.put_mov(EBP, 0);
	writer.put_mov(RCX, RSP);
	writer.put_call("stdcall");
	writer.put_sub(RCX, RSP);
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

TEST (writer_exec_cqo) {

	BufferWriter writer;
	writer.label("L1");
	writer.put_mov(RDX, 0);
	writer.put_mov(RAX, 0x8FFFFFFF'FFFFFFFF);

	writer.put_cqo();
	writer.put_mov(RAX, RDX);
	writer.put_ret();

	writer.label("L0");
	writer.put_mov(RDX, 0);
	writer.put_mov(RAX, 0x7FFFFFFF'FFFFFFFF);

	writer.put_cqo();
	writer.put_mov(RAX, RDX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u64("L1"), 0xFFFFFFFF'FFFFFFFF);
	CHECK(buffer.call_u64("L0"), 0);

}

TEST (writer_exec_xadd) {

	BufferWriter writer;

	writer.put_mov(RAX, 0);
	writer.put_mov(RBX, 70000);
	writer.put_mov(RCX, 30000);

	// RBX = RBX + RCX
	// RCX = RBX
	writer.put_xadd(RBX, RCX);

	writer.put_cmp(RCX, 70000);
	writer.put_jne("end");
	writer.put_mov(RAX, RBX);

	writer.label("end");
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u64(0), 100000);

}

TEST (writer_exec_xadd_mem) {

	BufferWriter writer;

	writer.label("L1");
	writer.put_qword(0x1111);

	writer.label("start");
	writer.put_mov(RAX, 0);
	writer.put_mov(R15, 0x2222);

	// L1 = L1 + R15
	// R15 = L1
	writer.put_xadd(ref("L1"), R15);

	writer.put_mov(R14, ref("L1"));
	writer.put_cmp(R14, 0x3333);
	writer.put_jne("end");
	writer.put_mov(RAX, R15);

	writer.label("end");
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u64("start"), 0x1111);

}

TEST (writer_exec_bswap_qword) {

	BufferWriter writer;
	writer.put_mov(R15, 0x11223344'55667788);
	writer.put_bswap(R15);
	writer.put_mov(RAX, R15);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u64(0), 0x88776655'44332211);

}

TEST (writer_exec_bswap_dword) {

	BufferWriter writer;
	writer.put_mov(R15D, 0x11223344);
	writer.put_bswap(R15D);
	writer.put_mov(EAX, R15D);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u32(0), 0x44332211);

}

TEST (writer_exec_cmpxchg) {

	BufferWriter writer;

	writer.put_mov(R15, 0x1000);
	writer.put_mov(RAX, 0x1000);
	writer.put_mov(R14, 0x42);

	writer.put_cmpxchg(R15, R14);
	writer.put_mov(RAX, R15);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u32(0), 0x42);

}

TEST (writer_exec_cmpxchg_mem) {

	BufferWriter writer;

	writer.label("L1");
	writer.put_qword(0x1000);

	writer.label("start");
	writer.put_mov(RAX, 0x1000);
	writer.put_mov(R14, 0x42);

	writer.put_cmpxchg(ref("L1"), R14);
	writer.put_mov(RAX, ref("L1"));
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	CHECK(buffer.call_u32("start"), 0x42);

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

TEST (writer_elf_execve_int) {

	using namespace asmio::elf;

	BufferWriter writer;

	writer.label("text").put_ascii("Hello World!\n");

	writer.label("strlen");
	writer.put_mov(RCX, RAX);
	writer.put_dec(RAX);
	writer.label("l_strlen_next");
	writer.put_inc(RAX);
	writer.put_cmp(ref<BYTE>(RAX), 0);
	writer.put_jne("l_strlen_next");
	writer.put_sub(RAX, RCX);
	writer.put_ret();

	writer.label("_start");
	writer.put_lea(RAX, "text");
	writer.put_call("strlen");
	writer.put_mov(RBX, RAX); // exit code
	writer.put_mov(RAX, 1); // sys_exit
	writer.put_int(0x80); // 32 bit syscall
	writer.put_ret();

	ElfBuffer file = writer.bake_elf(nullptr);
	int status;
	RunResult result = file.execute("memfd-elf-1", &status);

	CHECK(result, RunResult::SUCCESS);
	CHECK(status, 13);

}

TEST (writer_elf_execve_syscall) {

	using namespace asmio::elf;

	BufferWriter writer;

	writer.label("text").put_ascii("My beloved syscall!\n");

	writer.label("strlen");
	writer.put_mov(RCX, RAX);
	writer.put_dec(RAX);
	writer.label("l_strlen_next");
	writer.put_inc(RAX);
	writer.put_cmp(ref<BYTE>(RAX), 0);
	writer.put_jne("l_strlen_next");
	writer.put_sub(RAX, RCX);
	writer.put_ret();

	writer.label("_start");
	writer.put_lea(RAX, "text");
	writer.put_call("strlen");
	writer.put_mov(RDI, RAX); // exit code
	writer.put_mov(RAX, 60); // sys_exit
	writer.put_syscall();
	writer.put_ret();

	ElfBuffer file = writer.bake_elf(nullptr);
	int status;
	RunResult result = file.execute("memfd-elf-1", &status);

	CHECK(result, RunResult::SUCCESS);
	CHECK(status, 20);

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