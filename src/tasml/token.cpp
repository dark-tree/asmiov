
#include "token.hpp"

#include <util.hpp>

namespace tasml {

	int64_t Token::as_int() const {

		if (type == FLOAT) {
			return static_cast<int64_t>(as_float());
		}

		if (type != INT) {
			return -1;
		}

		try {
			return asmio::util::parse_int(raw.c_str());
		} catch(std::runtime_error& error) {
			throw std::runtime_error {"Internal lexer error, " + std::string {error.what()} + " while parsing int! In: '" + raw + "'"};
		}
	}

	long double Token::as_float() const {

		if (type == INT) {
			return static_cast<long double>(as_int());
		}

		if (type != FLOAT) {
			return -1.0f;
		}

		try {
			return asmio::util::parse_float(raw.c_str());
		} catch(std::runtime_error& error) {
			throw std::runtime_error {"Internal lexer error, " + std::string {error.what()} + " while parsing float! In: '" + raw + "'"};
		}
	}

	std::string Token::as_string() const {

		std::string output;
		output.reserve(raw.size() - 2);
		bool escape = false;

		for (int i = 1; i < (int) raw.size() - 1; i ++) {
			char chr = raw[i];

			if (escape) {
				escape = false;
				output.push_back(get_escaped(chr));
				continue;
			}

			if (chr == '\\') {
				escape = true;
				continue;
			}

			output += chr;
		}

		return output;
	}

	std::string Token::as_label() const {
		if (type != LABEL) {
			throw std::runtime_error {"Internal lexer error, can't convert non label into a label value!"};
		}

		return raw.substr(0, raw.length() - 1);
	}

}
