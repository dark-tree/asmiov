#pragma once

#include <external.hpp>
#include <macro.hpp>
#include <fstream>
#include <random>

template <typename T>
concept trivially_copyable = std::is_trivially_copyable_v<T>;

template <typename T, typename A>
concept castable = requires (const A& arg) { static_cast<T>(arg); };

#define EXIT_OK 0
#define EXIT_ERROR 1

#define ASMIOV_VERSION "1.0.0"
#define ASMIOV_SOURCE "https://github.com/dark-tree/asmiov"

/// Unsigned divide (round up)
#define UDIV_UP(a, b) (((a) + (b) - 1) / (b))

/// Align 'a' to a multiple of 'b'
#define ALIGN_UP(a, b) (UDIV_UP(a, b) * (b))

namespace asmio::util {

	inline std::string random_string(size_t length) {
		static const std::string_view alphabet = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
		static std::uniform_int_distribution dist(0, static_cast<int>(alphabet.size() - 1));
		thread_local std::mt19937 rng {std::random_device{} ()};

		std::string out;
		out.reserve(length);

		for (std::size_t i = 0; i < length; ++i) {
			out.push_back(alphabet[dist(rng)]);
		}

		return out;
	}

	inline void load_file_into(std::ifstream& file, std::string& string) {
		file.seekg(0, std::ios::end);
		string.reserve(file.tellg());
		file.seekg(0, std::ios::beg);
		string.assign(std::istreambuf_iterator {file}, std::istreambuf_iterator<char> {});
	}

	/// Unsigned divide (round up)
	template <std::integral T>
	constexpr auto divide_up(T a, T b) {
		return (a + b - 1) / b;
	}

	/// Align 'a' to a multiple of 'alignment'
	template <std::integral T>
	constexpr auto align_up(T a, T alignment) {
		return divide_up(a, alignment) * alignment;
	}

	/// Compute the number that needs to be added to 'a' so that it is a multiple of 'alignment'
	template <std::integral T>
	constexpr auto align_padding(T a, T alignment) {
		return align_up(a, alignment) - a;
	}

	/// Convert value to the given endian from the native system alignment
	template <std::integral T>
	constexpr auto native_to_endian(T value, std::endian endian) {
		return std::endian::native == endian ? value : std::byteswap(value);
	}

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
		std::ranges::transform(s, s.begin(), [] (const int c) noexcept -> int { return std::tolower(c); });
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
	constexpr int count_redundant_sign_bits(const int64_t value) {
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
		return __builtin_ctzll(~value); // ctz(~x) == cto(x)
	}

	template <std::integral T>
	constexpr T bit_fill(uint64_t count) {
		if (count >= sizeof(T) * 8) {
			return std::numeric_limits<T>::max();
		}

		return (1UL << count) - 1UL;
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

	constexpr uint64_t hash_djb2(const char* str, size_t bytes) {
		uint64_t hash = 5381;

		for (size_t i = 0; i < bytes; i ++) {
			hash = (hash << 5) + hash * 33 + str[i];
		}

		return hash;
	}

	constexpr uint64_t hash_tmix64(uint64_t x) {
		x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
		x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
		return x ^ (x >> 31);
	}

	constexpr int digit_value(char c) {
		if (c >= '0' && c <= '9') {
			return c - '0';
		}

		if (c >= 'a' && c <= 'f') {
			return c - 'a' + 10;
		}

		if (c >= 'A' && c <= 'F') {
			return c - 'A' + 10;
		}

		throw std::runtime_error {"Invalid digit '" + std::string(1, c) + "'"};
	}

	constexpr int64_t parse_int(const char* str) {

		int base = 10;
		size_t length = strlen(str);
		int64_t sign = 1;

		if (str[0] == '+') {
			str ++;
		} else if (str[0] == '-') {
			str ++;
			sign = -1;
		}

		if (length > 2 && str[0] == '0') {
			if (str[1] == 'x') { str += 2; base = 16; }
			else if (str[1] == 'o') { str += 2; base = 8;  }
			else if (str[1] == 'b') { str += 2; base = 2;  }
		}

		// update length as we could have changed starting point
		length = strlen(str);
		int64_t value = 0;

		for (size_t i = 0; i < length; i ++) {
			char c = str[i];

			if (c == '\'' || c == '_') {
				continue;
			}

			int next = digit_value(c);

			if (next >= base) {
				throw std::runtime_error {"invalid number format"};
			}

			value = (value * base) + next;
		}

		return value * sign;

	}

	inline int64_t parse_decimal(const std::string_view& str) {

		int64_t value;

		if (std::from_chars(str.data(), str.data() + str.size(), value).ec == std::errc {}) {
			return value;
		}

		throw std::runtime_error {"Can't parse '" + std::string(str) + "' as an integer!"};
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