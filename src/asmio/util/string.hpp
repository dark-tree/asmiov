#pragma once

#include <asmio/external.hpp>

namespace asmio::util {

	// https://stackoverflow.com/a/6500499
	inline std::string trim(const std::string& str) {
		int prefix = 0; // will point to the first non-space char
		int suffix = str.length() - 1;

		while (prefix < static_cast<int>(str.length())) {
			if (!std::isspace(str[prefix])) {
				break;
			}

			prefix ++;
		}

		while (suffix > 0) {
			if (!std::isspace(str[suffix])) {
				break;
			}

			suffix --;
		}

		int count = suffix - prefix + 1;

		if (count < 0) {
			count = 0;
		}

		return str.substr(prefix, count);
	}

	inline std::vector<std::string> normalize_strings(const std::vector<std::string>& strings) {
		std::vector<std::string> output;
		output.reserve(strings.size());

		for (auto& string : strings) {
			if (string.empty()) {
				continue;
			}

			output.push_back(trim(string));
		}

		return output;
	}

	// https://stackoverflow.com/a/46931770
	inline std::vector<std::string> split_string(const std::string& str, const std::string_view& delim) {
		size_t pos_start = 0, pos_end, delim_len = delim.length();
		std::string token;
		std::vector<std::string> res;

		while ((pos_end = str.find(delim, pos_start)) != std::string::npos) {
			token = str.substr (pos_start, pos_end - pos_start);
			pos_start = pos_end + delim_len;
			res.push_back (token);
		}

		res.push_back(str.substr(pos_start));
		return res;
	}

	// https://stackoverflow.com/a/46931770
	inline std::vector<std::string> split_string(const std::string& str, char delim = '\n') {
		std::vector<std::string> result;
		std::stringstream ss (str);
		size_t count = 0;

		for (char c : str) {
			if (c == delim) count ++;
		}

		result.reserve(count);
		std::string item;

		while (getline(ss, item, delim)) {
			result.push_back(item);
		}

		return result;
	}

	/// Generate random ASCII string of the given length
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

	/// Convert string to lower case
	inline std::string to_lower(std::string s) {
		std::ranges::transform(s, s.begin(), [] (const int c) noexcept -> int { return std::tolower(c); });
		return s;
	}

	/// Convert integer into hex string
	/// @see https://stackoverflow.com/a/5100745
	template<typename T>
	std::string to_hex(T value) {
		std::stringstream stream;
		stream << "0x" << std::setfill('0') << std::setw(sizeof(T)*2) << std::hex << value;
		return stream.str();
	}

	/// Get numerical value of a hexadecimal (or decimal) digit
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

	/// parse any number from string
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

	/// parse decimal number from string
	inline int64_t parse_decimal(const std::string_view& str) {

		int64_t value;

		if (std::from_chars(str.data(), str.data() + str.size(), value).ec == std::errc {}) {
			return value;
		}

		throw std::runtime_error {"Can't parse '" + std::string(str) + "' as an integer!"};
	}
	/// parse float number from string
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

	// https://codereview.stackexchange.com/a/22907
	inline std::vector<char> read_whole(const std::string& path) {
		std::ifstream ifs(path, std::ios::binary|std::ios::ate);

		if (!ifs.is_open() || ifs.bad()) {
			throw std::runtime_error {"Could not open file '" + path + "'"};
		}

		std::ifstream::pos_type pos = ifs.tellg();

		if (pos == 0) {
			return {};
		}

		std::vector<char> result(pos);

		ifs.seekg(0, std::ios::beg);
		ifs.read(result.data(), pos);

		return result;
	}

	inline void load_file_into(std::ifstream& file, std::string& string) {
		file.seekg(0, std::ios::end);
		string.reserve(file.tellg());
		file.seekg(0, std::ios::beg);
		string.assign(std::istreambuf_iterator {file}, std::istreambuf_iterator<char> {});
	}

}