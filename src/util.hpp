#pragma once

#include <external.hpp>

#define EXIT_OK 0
#define EXIT_ERROR 1

#define ASMIOV_VERSION "1.0.0"
#define ASMIOV_SOURCE "https://github.com/dark-tree/asmiov"

#define UDIV_UP(a, b) (((a) + (b) - 1) / (b))
#define ALIGN_UP(a, b) (UDIV_UP(a, b) * (b))

namespace asmio::util {

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
	bool contains(const T& container, const auto& value) {
		for (const auto& element : container) {
			if (element == value) return true;
		}

		return false;
	}

	inline std::string to_lower(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
		return s;
	}

	constexpr int min_bytes(uint64_t value) {
		if (value > 0xFFFFFFFF) return 8;
		if (value > 0xFFFF) return 4;
		if (value > 0xFF) return 2;

		return 1;
	}

	/**
	 * Check how many bits can be truncated from a signed number before
	 * it changed its value, assuming one bit is needed for the sign.
	 */
	constexpr int count_redundant_sign_bits(int64_t value) {
		return __builtin_clzll(value >= 0 ? value : ~value) - 1;
	}

	/**
	 * Check if given number signed number can be losslessly
	 * encoded in the given number of bits, taking into account the sign bits.
	 */
	constexpr bool is_signed_encodable(int64_t value, int64_t bits) {
		return (64 - count_redundant_sign_bits(value)) <= bits;
	}

	/**
	 * Count the number of 'ones' form the leading (most
	 * significant) side of a number.
	 */
	constexpr int count_trailing_ones(uint64_t value) {
		return __builtin_ctz(~value); // ctz(~x) == cto(x)
	}

	template <std::integral T>
	constexpr T bit_fill(uint64_t count) {
		if (count >= sizeof(T) * 8) {
			return std::numeric_limits<T>::max();
		}

		return (1 << count) - 1;
	}

	constexpr int min_sign_extended_bytes(int64_t value) {
		if ((value & 0xFFFF'FFFF'FFFF'FF80) == 0xFFFF'FFFF'FFFF'FF80) return 1; // 1 byte long negative
		if ((value & 0xFFFF'FFFF'FFFF'FF80) == 0x0000'0000'0000'0080) return 2; // 2 byte long positive
		if ((value & 0xFFFF'FFFF'FFFF'8000) == 0xFFFF'FFFF'FFFF'8000) return 2; // 2 byte long negative
		if ((value & 0xFFFF'FFFF'FFFF'8000) == 0x0000'0000'0000'8000) return 4; // 4 byte long positive
		if ((value & 0xFFFF'FFFF'8000'0000) == 0xFFFF'FFFF'8000'0000) return 4; // 4 byte long negative
		if ((value & 0xFFFF'FFFF'8000'0000) == 0x0000'0000'8000'0000) return 8; // 8 byte long positive

		// if we reached this point we know the value can't be sign extended
		// bu we still don't know how long the number is, for example 0 also can't be sign extended
		// so to fix this we invoke the unsigned version of this function
		return min_bytes(value);
	}

	/// Convert integer into hex string
	/// @see https://stackoverflow.com/a/5100745
	template<typename T>
	std::string to_hex(T value) {
		std::stringstream stream;
		stream << "0x" << std::setfill('0') << std::setw(sizeof(T)*2) << std::hex << value;
		return stream.str();
	}

	constexpr uint64_t hash_djb2(const char *str) {
		uint64_t hash = 5381;

		for (int c; (c = *str) != 0; str++) {
			hash = (hash << 5) + hash * 33 + c;
		}

		return hash;
	}

	inline int64_t parse_int(const char* str) {

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

	inline long double parse_float(const char* str) {

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