#pragma once

#include "external.hpp"
#include "predicate.hpp"
#include "token.hpp"

namespace tasml {

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
					throw_input_end(close);
				}

				return TokenStream {tokens, begin - 1, index - 1, name};
			}

		public:

			explicit TokenStream(const std::vector<Token>& tokens)
			: tokens(tokens), start(-1), end(tokens.size()), index(0) {}

			explicit TokenStream(const std::vector<Token>& tokens, long start, long end, const char* name)
			: tokens(tokens), start(start), end(end), index(start + 1), name(name) {}

		public:

			/// Returns the first token in this stream
			/// If that is not possible it returns the closest possible token
			const Token& first() const {
				if (start + 1 == end) {
					return tokens.at(start < 0 ? 0 : start);
				}

				return tokens.at(start + 1);
			}

			/// Throws a "unexpected end of scope" report, specifying the expectation with a TokenPredicate.
			/// Warning: this method does NOT check if the stream is actually empty! To do that use assertEmpty()
			void throw_input_end(const TokenPredicate& predicate) const {
				if (end < (int) tokens.size() && end - 1 > start) {
					throw std::runtime_error {"Unexpected end of " + std::string(name) + ", expected " + predicate.quoted() + " after " + tokens.at(end - 1).quoted() + " but got " + tokens.at(end).quoted()};
				}

				if (end - 1 > start) {
					throw std::runtime_error {"Unexpected end of input, expected " + predicate.quoted() + " after " + tokens.at(end - 1).quoted()};
				}

				throw std::runtime_error {"Unexpected end of input, expected " + predicate.quoted()};
			}

			/// Throws a generic "unexpected end of scope" report without specifying the expectation.
			/// Warning: this method does NOT check if the stream is actually empty! To do that use assertEmpty()
			void throw_input_end() const {
				if (end < (int) tokens.size()) {
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
			const Token& peek() const {
				if (index >= end) throw_input_end();
				return tokens[index];
			}

			/// Returns a reference to the previous token,
			/// if there was nothing to return it creates and throws a report.
			const Token& prev() const {
				if (index <= 0) throw std::runtime_error {"Unexpected start of " + std::string(name) + ", expected a preceding token"};
				return tokens[index - 1];
			}

			/// Consumes the next token, and returns a reference to it
			/// if there was nothing left to consume (stream was empty) it creates and throws a report.
			const Token& next() {
				if (index >= end) throw_input_end();
				return tokens[index ++];
			}

			/// Asserts that no tokens remain in this stream,
			/// otherwise it creates and throws a report.
			void assert_empty() const {
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
					throw_input_end(predicate);
				}

				const Token& token = peek();

				if (!predicate.test(token)) {
					throw std::runtime_error {"Unexpected token " + peek().quoted() + ", expected " + predicate.quoted()};
				}

				return next();
			}

			/// Rewind the state of this stream back to how it was when it was first created,
			/// if the stream is already in starting position no action will be taken.
			void rewind() {
				index = std::min(index, start + 1);
			}

			void terminal() {
				if (accept(";")) return; // semicolon
				if (empty()) return;        // last token
				if (index == 0) return;     // first token

				const Token& token = peek();
				const Token& last = prev();

				if (token.line == last.line) {
					throw std::runtime_error {"Unexpected token '" + token.raw + "', expected end of line or semicolon!"};
				}

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
			TokenStream expression(const char* name = "expression") {
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
			TokenStream statement(const char* name = "statement") {
				long begin = index;
				size_t line = peek().line;

				while (!empty()) {
					const Token& peeked = peek();

					if (peeked.line != line || peeked.raw == ";") {
						break;
					}

					const Token& consumed = next();

					// label is always the last token in a statement
					// note: label is not the same as a label reference
					if (consumed.type == Token::LABEL) {
						break;
					}
				}

				long finish = index;
				accept(";");

				return TokenStream {tokens, begin - 1, finish, name};
			}

			/// Consumes tokens until the end of line or a ';' is reached
			/// Returns a sub-stream of the consumed tokens, looks at the previous token to determine statement line
			TokenStream tail(const char* name = "statement") {
				long begin = index;
				size_t line = prev().line;

				while (!empty()) {
					const Token& token = peek();

					if (token.line != line || token.raw == ";") {
						break;
					}

					next();
				}

				long finish = index;
				accept(";");

				return TokenStream {tokens, begin - 1, finish, name};
			}

	};

}