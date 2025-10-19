#pragma once

#include "external.hpp"
#include "../asm/util.hpp"

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

		/// Get the byte value of a specific escale code suffix
		static constexpr signed char get_escaped(char chr) {
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
				default: return -1;
			}
		}

		/// Try to convert (cast) this token to a integer value
		int64_t as_int() const;

		/// Try to convert (cast) this token to a floating point value
		long double as_float() const;

		/// Try to convert (cast) this token to a string
		std::string as_string() const;

		/// Get the label string of a label (the part without ':')
		std::string as_label() const;

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