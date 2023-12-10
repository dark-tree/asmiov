
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

BEGIN(VSTL_MODE_LENIENT)