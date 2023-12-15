
#define DEBUG_MODE false

#include "lib/vstl.hpp"
#include "writer.hpp"

using namespace asmio::x86;

TEST(writer_basic) {

	BufferWriter writer;

	writer.put_mov(EAX, 5);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int eax = buffer.call();

	CHECK(eax, 5);

}

TEST(writer_simple_arithmetic) {

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
	int eax = buffer.call();

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
	int eax = buffer.call();

	CHECK(eax, 0xD);

}

TEST(writer_simple_push_pop) {

	BufferWriter writer;

	writer.put_mov(EAX, 9);
	writer.put_push(EAX);
	writer.put_mov(EAX, 7);
	writer.put_pop(ECX);
	writer.put_mov(EAX, ECX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int eax = buffer.call();

	CHECK(eax, 9);

}

TEST(writer_simple_movsx_movzx) {

	BufferWriter writer;

	writer.put_mov(CL, 9);
	writer.put_movzx(EDX, CL);
	writer.put_neg(DL);
	writer.put_movsx(EAX, DL);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int eax = buffer.call();

	CHECK(eax, -9);

}

TEST(writer_simple_lea) {

	int eax = 12, edx = 56, ecx = 60;

	BufferWriter writer;

	writer.put_mov(EAX, eax);
	writer.put_mov(EDX, edx);
	writer.put_mov(ECX, ecx);
	writer.put_lea(ECX, EDX + EAX * 2 - 5);
	writer.put_lea(EAX, ECX + EAX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call();

	CHECK(output, (edx + eax * 2 - 5) + eax);
}

TEST(writer_simple_add) {

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
	int output = buffer.call();

	CHECK(output, eax + edx + ecx + 5);

}

TEST(writer_simple_add_adc) {

	BufferWriter writer;

	writer.put_mov(EAX, 0xFFFFFFF0);
	writer.put_mov(EDX, 0x00000000);
	writer.put_add(EAX, 0x1F);
	writer.put_adc(EDX, 0);
	writer.put_xchg(EAX, EDX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call();

	CHECK(output, 0x1);

}

TEST(writer_simple_sub) {

	BufferWriter writer;

	writer.put_mov(EAX, 0xF);
	writer.put_mov(EDX, 0xA);
	writer.put_sub(EAX, EDX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call();

	CHECK(output, 0xF - 0xA);

}

TEST(writer_simple_sub_sbb) {

	BufferWriter writer;

	writer.put_mov(EAX, 0xA);
	writer.put_mov(EDX, 0x0);
	writer.put_sub(EAX, 0xB);
	writer.put_sbb(EDX, 0);
	writer.put_xchg(EAX, EDX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call();

	CHECK(output, -1);

}

TEST(writer_simple_stc_lahf_sahf) {

	BufferWriter writer;

	writer.put_mov(AH, 0);
	writer.put_sahf();
	writer.put_stc();
	writer.put_lahf();
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = (buffer.call() & 0xFF00) >> 8;

	ASSERT(!(output & 0b1000'0000)); // ZF
	ASSERT( (output & 0b0000'0001)); // CF
	ASSERT( (output & 0b0000'0010)); // always set to 1

}

TEST (writer_simple_cmp_lahf) {

	BufferWriter writer;

	writer.put_mov(EAX, 0xA);
	writer.put_mov(EDX, 0xE);
	writer.put_cmp(EAX, EDX);
	writer.put_lahf();
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = (buffer.call() & 0xFF00) >> 8;

	ASSERT(output & 0b1000'0000); // ZF
	ASSERT(output & 0b0000'0001); // CF
	ASSERT(output & 0b0000'0010); // always set to 1

}

TEST(writer_simple_mul) {

	BufferWriter writer;

	writer.put_mov(EAX, 0xA);
	writer.put_mov(EDX, 0xB);
	writer.put_mul(EDX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call();

	CHECK(output, 0xA * 0xB);

}

TEST(writer_simple_mul_imul) {

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
	int output = buffer.call();

	CHECK(output, 7 * 5 * 3 * 2 * 11);

}

TEST(writer_simple_div_idiv) {

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
	int output = buffer.call();

	CHECK(output, 50 / 5 / -2);

}

TEST(writer_simple_or) {

	BufferWriter writer;

	writer.put_mov(EAX, 0b0000'0110);
	writer.put_mov(EDX, 0b0101'0000);
	writer.put_or(EAX,  0b0000'1100);
	writer.put_or(EAX, EDX);
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call();

	CHECK(output, 0b0101'1110);

}

TEST(writer_simple_and_not_xor_or) {

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
	int output = buffer.call();

	CHECK(output, 0b1100'0100'1111);

}

TEST(writer_simple_aam) {

	BufferWriter writer;

	writer.put_mov(EAX, 27);
	writer.put_aam(); // AL -> 7, AH -> 2
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call();

	CHECK(output, 0x0207);

}

TEST(writer_simple_aad) {

	BufferWriter writer;

	writer.put_mov(EAX, 0x0509);
	writer.put_aad(); // AL -> 59, AH -> 0
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int output = buffer.call();

	CHECK(output, 59);

}

BEGIN(VSTL_MODE_LENIENT)