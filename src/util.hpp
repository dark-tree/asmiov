#pragma once

#include "external.hpp"

#define ASMIOV_VERSION "1.0.0"
#define ASMIOV_SOURCE "https://github.com/dark-tree/asmiov"

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

	template<typename T>
	inline bool contains(const T& container, const auto& value) {
		for (const auto& element : container) {
			if (element == value) return true;
		}

		return false;
	}

	inline std::string str_tolower(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
		return s;
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

	inline int64_t parseInt(const char* str) {

		int base = 10;
		size_t length = strlen(str);
		const char* digits = str;

		if (length > 2 && str[0] == '0') {
			if (str[1] == 'x') { digits = str + 2; base = 16; }
			if (str[1] == 'o') { digits = str + 2; base = 8;  }
			if (str[1] == 'b') { digits = str + 2; base = 2;  }
		}

		size_t offset;
		int64_t value;

		try {
			value = std::stoll(digits, &offset, base);
		} catch(...) {
			throw std::runtime_error {"exception thrown"};
		}

		if (offset != length - (base != 10 ? 2 : 0)) {
			throw std::runtime_error {"some input ignored"};
		}

		return value;

	}

	inline long double parseFloat(const char* str) {

		size_t offset;
		long double value;

		try {
			value = std::stold(str, &offset);
		} catch(...) {
			throw std::runtime_error {"exception thrown"};
		}

		if (offset != strlen(str)) {
			throw std::runtime_error {"some input ignored"};
		}

		return value;

	}

}