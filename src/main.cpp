#include <iostream>
#include <vector>
#include <csignal>

#include "writer.hpp"

using namespace asmio::x86;

static void handler(int sig, siginfo_t* si, void* unused) {
	printf("uga");
}

int main() {

	struct sigaction action;
	action.sa_flags = SA_SIGINFO;
	sigemptyset(&action.sa_mask);
	action.sa_sigaction = handler;

	sigaction(SIGSEGV, &action, nullptr);
	sigaction(SIGILL, &action, nullptr);

	BufferWriter writer;

	writer.put_nop();
	writer.put_mov(EDX, 5);
	writer.put_inc(EDX);
	writer.put_mov(EAX, EDX);
	writer.put_inc(EAX);
	writer.put_nop();
	writer.put_ret();

//	writer.put_byte(0xb8);
//	writer.put_byte(0x05);
//	writer.put_byte(0x00);
//	writer.put_byte(0x00);
//	writer.put_byte(0x00);
//	writer.put_byte(0xc3);

	ExecutableBuffer buffer = writer.bake();
	int eax = buffer.call();

	std::cout << "Hello, World! " << eax << std::endl;
	return 0;
}
