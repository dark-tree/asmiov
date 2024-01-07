#pragma once

#include "external.hpp"
#include "predicate.hpp"
#include "token.hpp"

class TokenStream {

	private:

		// both 'start' and 'end' are exclusive
		const std::vector<Token>& tokens;
		long start, end;
		long index;
		const char* name = "scope";

		TokenStream block(const TokenPredicate& open, const TokenPredicate& close, const char* name) {
			long begin = index;
			long depth = 1;

			while (index < end) {
				if (accept(open)) {
					depth ++;

					if (depth == 0) break;
					continue;
				}

				if (accept(close)) {
					depth --;

					if (depth == 0) break;
					continue;
				}

				next();
			}

			if (depth != 0) {
				raiseInputEnd(close);
			}

			return TokenStream {tokens, begin - 1, index - 1, name};
		}

	public:

		explicit TokenStream(const std::vector<Token>& tokens)
		: tokens(tokens), start(-1), end(tokens.size()), index(0) {}

		explicit TokenStream(const std::vector<Token>& tokens, long start, long end, const char* name)
		: tokens(tokens), start(start), end(end), index(start + 1), name(name) {}

	public:

		const Token& first() {
			if (start + 1 == end) {
				return tokens.at(start < 0 ? 0 : start);
			}

			return tokens.at(start + 1);
		}

		/// Throws a "unexpected end of scope" report, specifying the expectation with a TokenPredicate.
		/// Warning: this method does NOT check if the stream is actually empty! To do that use assertEmpty()
		void raiseInputEnd(const TokenPredicate& predicate) {
			if (end < tokens.size() && end - 1 > start) {
				throw std::runtime_error {"Unexpected end of " + std::string(name) + ", expected " + predicate.quoted() + " after " + tokens.at(end - 1).quoted() + " but got " + tokens.at(end).quoted()};
			}

			if (end - 1 > start) {
				throw std::runtime_error {"Unexpected end of input, expected " + predicate.quoted() + " after " + tokens.at(end - 1).quoted()};
			}

			throw std::runtime_error {"Unexpected end of input, expected " + predicate.quoted()};
		}

		/// Throws a generic "unexpected end of scope" report without specifying the expectation.
		/// Warning: this method does NOT check if the stream is actually empty! To do that use assertEmpty()
		void raiseInputEnd() {
			if (end < tokens.size()) {
				throw std::runtime_error {"Unexpected end of " + std::string(name)};
			}

			if (end - 1 > start) {
				throw std::runtime_error {"Unexpected end of input after " + tokens.at(end - 1).quoted()};
			}

			throw std::runtime_error {"Unexpected end of input"};
		}

		/// checks if the stream is empty
		bool empty() const {
			return index >= end;
		}

		/// Returns a reference to the next token without consuming it,
		/// if there was nothing left to consume (stream was empty) it creates and throws a report.
		const Token& peek() {
			if (index >= end) raiseInputEnd();
			return tokens[index];
		}

		/// Consumes the next token, and returns a reference to it
		/// if there was nothing left to consume (stream was empty) it creates and throws a report.
		const Token& next() {
			if (index >= end) raiseInputEnd();
			return tokens[index ++];
		}

		/// Asserts that no tokens remain in this stream,
		/// otherwise it creates and throws a report.
		const void assertEmpty() {
			if (!empty()) {
				throw std::runtime_error {"Unexpected token " + peek().quoted() + ", expected end of " + name};
			}
		}

		/// Checks if the next token matches the given predicate, if yes then the token is consumed.
		/// Returns a pointer to the matched token, or nullptr if no token was matched.
		const Token* accept(const TokenPredicate& predicate) {
			if (!empty() && predicate.test(peek())) {
				return &next();
			}

			return nullptr;
		}

		/// Consumes the next token and asserts that it matches the given predicate,
		/// otherwise it creates and throws a report.
		const Token& expect(const TokenPredicate& predicate) {
			if (index >= end) {
				raiseInputEnd(predicate);
			}

			const Token& token = peek();

			if (!predicate.test(token)) {
				throw std::runtime_error {"Unexpected token " + peek().quoted() + ", expected " + predicate.quoted()};
			}

			return next();
		}

		/// Helper for creating bracket-bound sub-streams, takes two strings, first the pattern (e.g. "{}" or "()")
		/// and second the name for the sub-stream. Expects the opening bracket to have already been consumed!
		TokenStream block(const char str[2], const char* name) {
			if (str[0] == 0 || str[1] == 0 || str[2] != 0) {
				throw std::runtime_error {"Invalid block pattern length!"};
			}

			const char open[2] = {str[0], 0}, close[2] = {str[1], 0};
			return block(open, close, name);
		}

		/// Consumes tokens until the end of input or a ',' is reached
		/// Returns a sub-stream of the consumed tokens
		TokenStream expression(const char* name) {
			long begin = index;

			while (!empty()) {
				const Token& token = peek();

				if (token.raw == ",") {
					break;
				} else next();
			}

			long finish = index;
			accept(",");

			return TokenStream {tokens, begin - 1, finish, name};
		}

		/// Consumes tokens until the end of line or a ';' is reached
		/// Returns a sub-stream of the consumed tokens
		TokenStream statement(const char* name) {
			long begin = index;
			long line = peek().line;

			while (!empty()) {
				const Token& token = peek();

				if (token.line != line || token.raw == ";") {
					break;
				} else next();
			}

			long finish = index;
			accept(";");

			return TokenStream {tokens, begin - 1, finish, name};
		}

};