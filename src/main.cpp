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
	writer.put_rol(EDX, 3);
	writer.put_inc(EDX);
	writer.put_mov(EAX, EDX);
	writer.put_inc(EAX);
	writer.put_neg(EAX);
	writer.put_mov(CL, 2);
	writer.put_sar(EAX, CL);
	writer.put_nop();
	writer.put_ret();

	ExecutableBuffer buffer = writer.bake();
	int eax = buffer.call();

	std::cout << "Hello, World! " << eax << std::endl;
	return 0;
}
