#pragma once

#include <util.hpp>

#define INST void
#define RETURN_TRANSIENT(T, format) {volatile T tmp; asm("" : format (tmp)); return tmp;}

extern "C" {
	extern int x86_check_mode();
	extern int x86_call_into(uint32_t (*)());
	extern int x86_switch_mode(uint32_t (*)());
}