#pragma once

#define UDIV_UP(a, b) (((a) + (b) - 1) / (b))
#define ALIGN_UP(a, b) (UDIV_UP(a, b) * (b))

int min_bytes(uint64_t value) {
	if (value > 0xFFFFFFFF) return 8;
	if (value > 0xFFFF) return 4;
	if (value > 0xFF) return 2;

	return 1;
}

extern "C" {
	extern int x86_check_mode();
	extern bool x86_switch_mode(int cs, bool (*f)());
}