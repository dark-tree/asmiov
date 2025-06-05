# Asmiov
x86-64 assembler with support for source file parsing (NASM-like Intel syntax) and JIT 
code generation using the build-in C++ API with syntax closely matching the source format.
The project consists of a `tasml` CLI assembler and a `test` unit test runner. 

The code unique to the CLI utility is separated from the rest of 
the library and located in the `/src/tasml`. The `/src/file` contains binary file IO, and `/src/asm` 
implements the assembler itself.

## Utility
The example of the syntax used by `tasml` utility, the API version of the exact same code can be seen below.
```asm
text:
	byte "Hello!", 0

strlen:
	mov rcx, /* inline comments! */ rax
	dec rax

	l_strlen_next:
		inc rax
		cmp byte [rax], 0
	jne @l_strlen_next

	sub rax, rcx
	ret

_start:
	lea rax, @text
	call @strlen
	mov edi, rax // exit code
	mov rax, 60 // sys_exit
	syscall
```

To assemble this code the following command can be used,
assuming the code is saved in file `test.s`
```bash
# TASML generates an executable ELF file
tasml -o a.out test.s
```

## Library
```C++
#include "asm/x86/writer.hpp"
#include "asm/x86/emitter.hpp"
#include "file/elf/buffer.hpp"

int main() {

	using namespace asmio::elf;

	BufferWriter writer;

	writer.label("text");
	writer.put_ascii("Hello!");

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

	ElfBuffer file = writer.bake_elf(nullptr);

	int status;
	RunResult result = file.execute("memfd-elf-1", &status);

	return status;
	
}


```