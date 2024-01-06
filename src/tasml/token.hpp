#pragma once

#include "external.hpp"

struct Token {

	/// see TypeMask
	enum Type : uint32_t {
		FLOAT     = 0b0001'10,
		INT       = 0b0010'10,
		STRING    = 0b0011'00,
		NAME      = 0b0100'00,
		LABEL     = 0b0101'00,
		REFERENCE = 0b0110'00,
		SYMBOL    = 0b0111'01,
		OPERATOR  = 0b1000'01,
		//                 ||
		//                 |\_ symbolic type
		//                 \__ numeric type

		INVALID = 0
	};

	int64_t parseInt() const {

		if (type == FLOAT) {
			return (int64_t) parseFloat();
		}

		if (type != INT) {
			return -1;
		}

		int base = 10;
		std::string digits;

		if (raw.size() <= 2) { digits = raw; base = 10; }
		else if (raw[0] == '0' && raw[1] == 'x') { digits = raw.substr(2); base = 16; }
		else if (raw[0] == '0' && raw[1] == 'o') { digits = raw.substr(2); base = 8; }
		else if (raw[0] == '0' && raw[1] == 'b') { digits = raw.substr(2); base = 2; }
		else { digits = raw; }

		size_t offset;
		int64_t value;

		try {
			value = std::stoll(digits, &offset, base);
		} catch(...) {
			throw std::runtime_error {"Internal lexer error, exception thrown while parsing int! In: '" + raw + "'"};
		}

		if (offset != digits.size()) {
			throw std::runtime_error {"Internal lexer error, some input ignored while parsing int! In: '" + raw + "'"};
		}

		return value;

	}

	double parseFloat() const {

		if (type == INT) {
			return (double) parseInt();
		}

		if (type != FLOAT) {
			return -1.0f;
		}

		size_t offset;
		long double value;

		try {
			value = std::stold(raw, &offset);
		} catch(...) {
			throw std::runtime_error {"Internal lexer error, exception thrown while parsing float! In: '" + raw + "'"};
		}

		if (offset != raw.size()) {
			throw std::runtime_error {"Internal lexer error, some input ignored while parsing float! In: '" + raw + "'"};
		}

		return (double) value;

	}

	const size_t line;
	const size_t column;
	const size_t offset;
	const size_t length;
	const std::string raw;
	const Type type;
	const short weight;

	// implicit length constructor
	Token(size_t line, size_t column, size_t offset, const std::string& raw, Type type, short weight)
	: Token(line, column, offset, raw.length(), raw, type, weight) {}

	// explicit length constructor
	Token(size_t line, size_t column, size_t offset, size_t length, const std::string& raw, Type type, short weight)
	: line(line), column(column), offset(offset), length(length), raw(raw), type(type), weight(weight) {}

	std::string cooked() const {
		if (type == STRING) return raw.substr(1, raw.length() - 2);
		if (type == LABEL) return raw.substr(0, raw.length() - 1);
		if (type == REFERENCE) return raw.substr(1, raw.length() - 1);
		return raw;
	}

	std::string quoted() const {
		return "'" + raw + "'";
	}

};