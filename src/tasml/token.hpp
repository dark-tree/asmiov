#pragma once

#include "external.hpp"
#include "util.hpp"

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

	int64_t parseInt() const {

		if (type == FLOAT) {
			return (int64_t) parseFloat();
		}

		if (type != INT) {
			return -1;
		}

		try {
			return asmio::parseInt(raw.c_str());
		} catch(std::runtime_error& error) {
			throw std::runtime_error {"Internal lexer error, " + std::string {error.what()} + " while parsing int! In: '" + raw + "'"};
		}
	}

	double parseFloat() const {

		if (type == INT) {
			return (double) parseInt();
		}

		if (type != FLOAT) {
			return -1.0f;
		}

		try {
			return (double) asmio::parseFloat(raw.c_str());
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

				if (chr == '\\') {
					output.push_back('\\');
				}

				if (chr == '"') {
					output.push_back('"');
				}

				if (chr == 'n') {
					output.push_back('\n');
				}

				if (chr == 't') {
					output.push_back('\t');
				}

				if (chr == 'r') {
					output.push_back('\r');
				}

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

//	std::string parseReference() const {
//		return raw.substr(1, raw.length() - 1);
//	}

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