#pragma once

#include "external.hpp"

#define UDIV_UP(a, b) (((a) + (b) - 1) / (b))
#define ALIGN_UP(a, b) (UDIV_UP(a, b) * (b))

namespace asmio {

	template<typename T>
	auto get_int_or(T value) {
		if constexpr (std::is_pointer_v<T>) return 0; else return value;
	}

	template<typename T>
	auto get_ptr_or(T value) {
		if constexpr (std::is_pointer_v<T>) return value; else return nullptr;
	}

	template<typename T>
	void insert_buffer(std::vector<T>& vec, T* buffer, size_t count) {
		vec.insert(vec.end(), buffer, buffer + count);
	}

	template<typename T>
	size_t insert_padding(std::vector<T>& vec, size_t count) {
		const size_t length = vec.size();
		vec.resize(vec.size() + count);
		return length;
	}

	template<typename S, typename T>
	size_t insert_struct(std::vector<T>& vec, size_t count = 1) {
		return insert_padding(vec, sizeof(S) * count);
	}

	template<typename S, typename T>
	S* buffer_view(std::vector<T>& vec, size_t offset) {
		return (S*) (vec.data() + offset);
	}

	inline int min_bytes(uint64_t value) {
		if (value > 0xFFFFFFFF) return 8;
		if (value > 0xFFFF) return 4;
		if (value > 0xFF) return 2;

		return 1;
	}

	inline uint64_t hash_djb2(const char *str) {
		uint64_t hash = 5381;

		for (int c; (c = *str) != 0; str++) {
			hash = (hash << 5) + hash * 33 + c;
		}

		return hash;
	}

}