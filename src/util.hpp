#pragma once

#define UDIV_UP(a, b) (((a) + (b) - 1) / (b))
#define ALIGN_UP(a, b) (UDIV_UP(a, b) * (b))

int min_bytes(uint64_t value) {
	if (value > 0xFFFFFFFF) return 8;
	if (value > 0xFFFF) return 4;
	if (value > 0xFF) return 2;

	return 1;
}



uint64_t hash_djb2(const char* str) {
	uint64_t hash = 5381;

	for (int c; (c = *str) != 0; str ++) {
		hash = (hash << 5) + hash * 33 + c;
	}

	return hash;
}

extern "C" {
	extern int x86_check_mode();
	extern int x86_switch_mode(int, uint32_t (*)());
}