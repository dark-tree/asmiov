#pragma once

#include "external.hpp"
#include "util.hpp"

namespace tasml {

	struct Token {

		enum Type : uint32_t {
			INVALID   = 0,
			FLOAT     = 1,
			INT       = 2,
			STRING    = 3,
			NAME      = 4,
			LABEL     = 5,
			REFERENCE = 6,
			SYMBOL    = 7,
			OPERATOR  = 8,
		};

		static inline signed char getEscapedValue(char chr) {
			switch (chr) {
				case 'n': return '\n'; // new line
				case 't': return '\t'; // tab
				case '0': return '\0'; // null byte
				case 'r': return '\r'; // carriage return
				case 'v': return '\v'; // vertical tab
				case 'a': return '\a'; // bell
				case 'e': return 27;   // ascii escape
				case '\\': return '\\';
				case '"': return '"';
				case '\'': return '\'';
			}

			return -1;
		}

		int64_t parseInt() const {

			if (type == FLOAT) {
				return (int64_t) parseFloat();
			}

			if (type != INT) {
				return -1;
			}

			try {
				return asmio::util::parseInt(raw.c_str());
			} catch(std::runtime_error& error) {
				throw std::runtime_error {"Internal lexer error, " + std::string {error.what()} + " while parsing int! In: '" + raw + "'"};
			}
		}

		long double parseFloat() const {

			if (type == INT) {
				return (double) parseInt();
			}

			if (type != FLOAT) {
				return -1.0f;
			}

			try {
				return asmio::util::parseFloat(raw.c_str());
			} catch(std::runtime_error& error) {
				throw std::runtime_error {"Internal lexer error, " + std::string {error.what()} + " while parsing float! In: '" + raw + "'"};
			}
		}

		std::string parseString() const {

			std::string output;
			output.reserve(raw.size() - 2);
			bool escape = false;

			for (int i = 1; i < raw.size() - 1; i ++) {
				char chr = raw[i];

				if (escape) {
					escape = false;
					output.push_back(getEscapedValue(chr));
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

		std::string parseLabel() const {
			return raw.substr(0, raw.length() - 1);
		}

		const size_t line;
		const size_t column;
		const size_t offset;
		const size_t length;
		const std::string raw;
		const Type type;

		// implicit length constructor
		Token(size_t line, size_t column, size_t offset, const std::string& raw, Type type)
		: Token(line, column, offset, raw.length(), raw, type) {}

		// explicit length constructor
		Token(size_t line, size_t column, size_t offset, size_t length, const std::string& raw, Type type)
		: line(line), column(column), offset(offset), length(length), raw(raw), type(type) {}

		std::string quoted() const {
			return "'" + raw + "'";
		}

	};

}