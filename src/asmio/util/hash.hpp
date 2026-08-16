#pragma once

#include <asmio/external.hpp>

namespace asmio::util {

	constexpr uint64_t djb2(const char* str, size_t bytes) {
		uint64_t hash = 5381;

		for (size_t i = 0; i < bytes; i ++) {
			hash = (hash << 5) + hash * 33 + str[i];
		}

		if (hash == 0) {
			return 1;
		}

		return hash;
	}

	constexpr uint64_t tmix64(uint64_t x) {
		x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
		x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
		x = (x ^ (x >> 31));

		if (x == 0) {
			return 1;
		}

		return x;
	}

	template <typename T>
	constexpr uint64_t djb2(const std::vector<T>& data) {
		return hash_djb2(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(T));
	}

}