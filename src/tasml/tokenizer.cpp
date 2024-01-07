#include "tokenizer.hpp"
#include "util.hpp"

// private library
#include <regex>

#define SKIP(count) i += (count); column += (count);

namespace asmio {

	static constexpr std::array symbols   {';', '{', '}', '(', ')', '[', ']', ','};
	static constexpr std::array operators {'+', '-', '*', '/', '%', '&', '|', '^'};

	inline bool isSpace(char chr) {
		return chr <= ' ';
	}

	inline bool isSymbol(char chr) {
		return contains(symbols, chr);
	}

	inline bool isOperator(char chr) {
		return contains(operators, chr);
	}

	inline bool isBreak(char chr) {
		return isSymbol(chr) || isOperator(chr);
	}

	Token::Type categorize(const std::string& raw) {
		static const std::regex floating_re {R"(^\d+\.\d+$)"};
		static const std::regex integer_re {R"(^([0-9]+)|(0x[0-9a-fA-F]+)|(0b[01]+)|(0o[0-7]+)$)"};
		static const std::regex string_re {R"(^".*"$)"};
		static const std::regex name_re {R"(^[A-Za-z_.$](\w|[.$])*$)"};
		static const std::regex label_re {R"(^[A-Za-z_.$](\w|[.$])*:$)"};
		static const std::regex reference_re {R"(^@[A-Za-z_.$](\w|[.$])*$)"};

		if (std::regex_match(raw, floating_re)) return Token::FLOAT;
		if (std::regex_match(raw, integer_re)) return Token::INT;
		if (std::regex_match(raw, string_re)) return Token::STRING;
		if (std::regex_match(raw, name_re)) return Token::NAME;
		if (std::regex_match(raw, label_re)) return Token::LABEL;
		if (std::regex_match(raw, reference_re)) return Token::REFERENCE;

		if (raw.length() == 1) {
			char chr = raw[0];

			if (isSymbol(chr)) return Token::SYMBOL;
			if (isOperator(chr)) return Token::OPERATOR;
		}

		return Token::INVALID;
	}


	std::vector<Token> tokenize(ErrorHandler& reporter, const std::string& input) {

		size_t size = input.size();
		int32_t line = 1, column = 0, start = 0, offset = 0;

		std::vector<Token> tokens;
		std::string token = "";

		bool in_string = false;
		bool in_comment = false;
		bool in_multiline_comment = false;

		auto submit = [&] (const std::string& raw, bool silent = false) {
			Token::Type type = categorize(raw);
			tokens.emplace_back(line, start, offset, raw, type);

			if (type == Token::INVALID && !silent) {
				reporter.error(line, start, "Unknown token '" + raw + "'");
			}
		};

		for (int i = 0; i < size; i ++) {
			column ++;

			char c = input[i];
			char n = (i + 1 >= size) ? '\0' : input[i + 1];

			if (c == '\n') {
				if (in_string) {
					reporter.error(line, start, "Unexpected end of line, expected end of string");
					submit(token, true);
					in_string = false;
					token = "";
				}

				if (!token.empty()) {
					submit(token);
				}

				line ++;
				column = 0;
				in_comment = false;
				token = "";
				continue;
			}

			if (in_comment) {
				continue;
			}


			if (in_multiline_comment) {
				if (c == '*' && n == '/') {
					// End of multi-line comment
					in_multiline_comment = false;
					token = "";
					SKIP(1); // Skip over the '/' character
				}

				continue;
			}

			// handle strings
			if (c == '"') {
				in_string = !in_string;
				token += c;

				if (!in_string) {
					submit(token);
					token = "";
				}

				start = column;
				offset = i;
				continue;
			}

			// add string content
			if (in_string) {
				token += c;

				if (c == '\\') {
					// TODO: checkEscapeCode(line, column, n);
					token += n;
					SKIP(1); // skip the escaped char
				}
				continue;
			}

			// handle operators and end tokens
			if (isSpace(c) || isBreak(c) || (c == '/' && n == '/') || (c == '/' && n == '*')) {

				if (!token.empty()) {
					submit(token);
					token = "";
				}

				// inline comment start
				if (c == '/' && n == '/') {
					in_comment = true;
					continue;
				}

				// multi-line comment start
				if (c == '/' && n == '*') {
					in_multiline_comment = true;
					SKIP(1); // skip over the '*' character
					continue;
				}

				if (!isSpace(c)) {
					start = column;
					offset = i;
					submit({c});
				}

			} else {
				if (token.empty()) {
					start = column;
					offset = i;
				}

				token += c;
			}

		}

		// behave as if there was an extra whitespace at the end
		column ++;

		if (in_multiline_comment) {
			reporter.warn(line, start, "Unexpected end of input, expected end of multiline comment");
		}

		if (in_string) {
			reporter.error(line, start, "Unexpected end of input, expected end of string");
			submit(token, true);
			token = "";
		}

		if (!token.empty()) {
			submit(token);
		}

		return tokens;

	}

}