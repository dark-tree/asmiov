#pragma once

#include "token.hpp"

class TokenPredicate {

	private:

		const char* text;
		const Token::Type type;
		const bool literal;

		const char* typestr() const {
			if (type == Token::FLOAT) return "floating point";
			if (type == Token::INT) return "integer";
			if (type == Token::STRING) return "string";
			if (type == Token::NAME) return "name";
			if (type == Token::LABEL) return "label definition";
			if (type == Token::SYMBOL) return "symbol";
			if (type == Token::REFERENCE) return "label reference";
			if (type == Token::OPERATOR) return "operator";

			return "invalid";
		}

	public:

		TokenPredicate(const char* text) :
		text(text), type(Token::INVALID), literal(true) {}

		TokenPredicate(Token::Type type) :
		text(nullptr), type(type), literal(false) {}

		bool test(const Token& token) const {
			return literal ? (token.raw == text) : (token.type == type);
		}

		std::string quoted() const {
			return literal ? text : typestr();
		}

};
