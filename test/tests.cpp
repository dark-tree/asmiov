
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

BEGIN(VSTL_MODE_LENIENT)