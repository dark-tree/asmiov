#include "module.hpp"
#include "writer.hpp"

#include "src/tasml/stream.hpp"

namespace asmio::x86 {

	using namespace tasml;

	static int token_to_sizing(const Token* token) {
		if (token == nullptr || token->type != Token::NAME) return -1;
		std::string raw = util::to_lower(token->raw);

		static std::unordered_map<std::string, int> types = {
			{"byte", BYTE},
			{"word", WORD},
			{"dword", DWORD},
			{"qword", QWORD},
			{"tword", TWORD},

			{"float", DWORD},
			{"double", QWORD},
			{"real", TWORD},
		};

		auto it = types.find(raw);

		if (it == types.end()) {
			return -1;
		}

		return it->second;
	}

	static Registry token_to_register(const Token* token) {
		if (token == nullptr || token->type != Token::NAME) return UNSET;
		std::string raw = util::to_lower(token->raw);

		if (raw == "eax") return EAX;
		if (raw == "ax") return AX;
		if (raw == "al") return AL;
		if (raw == "ah") return AH;
		if (raw == "ebx") return EBX;
		if (raw == "bx") return BX;
		if (raw == "bl") return BL;
		if (raw == "bh") return BH;
		if (raw == "ecx") return ECX;
		if (raw == "cx") return CX;
		if (raw == "cl") return CL;
		if (raw == "ch") return CH;
		if (raw == "edx") return EDX;
		if (raw == "dx") return DX;
		if (raw == "dl") return DL;
		if (raw == "dh") return DH;
		if (raw == "esi") return ESI;
		if (raw == "si") return SI;
		if (raw == "edi") return EDI;
		if (raw == "di") return DI;
		if (raw == "ebp") return EBP;
		if (raw == "bp") return BP;
		if (raw == "esp") return ESP;
		if (raw == "sp") return SP;
		if (raw == "st") return ST;
		if (raw == "spl") return SPL;
		if (raw == "bpl") return BPL;
		if (raw == "sil") return SIL;
		if (raw == "dil") return DIL;
		if (raw == "rax") return RAX;
		if (raw == "rbx") return RBX;
		if (raw == "rcx") return RCX;
		if (raw == "rdx") return RDX;
		if (raw == "rsi") return RSI;
		if (raw == "rdi") return RDI;
		if (raw == "rbp") return RBP;
		if (raw == "rsp") return RSP;
		if (raw == "r8l") return R8L;
		if (raw == "r8w") return R8W;
		if (raw == "r8d") return R8D;
		if (raw == "r8") return R8;
		if (raw == "r9l") return R9L;
		if (raw == "r9w") return R9W;
		if (raw == "r9d") return R9D;
		if (raw == "r9") return R9;
		if (raw == "r10l") return R10L;
		if (raw == "r10w") return R10W;
		if (raw == "r10d") return R10D;
		if (raw == "r10") return R10;
		if (raw == "r11l") return R11L;
		if (raw == "r11w") return R11W;
		if (raw == "r11d") return R11D;
		if (raw == "r11") return R11;
		if (raw == "r12l") return R12L;
		if (raw == "r12w") return R12W;
		if (raw == "r12d") return R12D;
		if (raw == "r12") return R12;
		if (raw == "r31l") return R13L;
		if (raw == "r13w") return R13W;
		if (raw == "r13d") return R13D;
		if (raw == "r13") return R13;
		if (raw == "r14l") return R14L;
		if (raw == "r14w") return R14W;
		if (raw == "r14d") return R14D;
		if (raw == "r14") return R14;
		if (raw == "r15l") return R15L;
		if (raw == "r15w") return R15W;
		if (raw == "r15d") return R15D;
		if (raw == "r15") return R15;

		throw std::runtime_error {"Unknown registry " + token->quoted()};
	}

	static Location parse_expression(TokenStream stream) {
		const Token* base  = nullptr;
		const Token* index = nullptr;
		const Token* scale = nullptr;
		const Token* label = nullptr;
		long offset = 0;

		// the next token MUST be this, else parse throws
		const char* expect = nullptr;

		// the operator for the next offset integer
		char op = '+';

		enum {
			BASE   = 0,
			INDEX  = 1,
			SCALE  = 2,
			LABEL  = 3,
			OFFSET = 4
		} state = BASE;

		// expression can't be empty
		if (stream.empty()) {
			stream.throw_input_end();
		}

		while (!stream.empty()) {

			// handle forced tokens
			if (expect != nullptr) {
				stream.expect(expect);
				expect = nullptr;
				continue;
			}

			if (state == BASE) {
				const Token* token = stream.accept(Token::NAME);

				// no base name, skip
				if (token == nullptr) {
					state = LABEL;
					continue;
				}

				// if the next token is '*' this is index not base
				if (stream.accept("*")) {
					state = SCALE;
					index = token;
					continue;
				}

				// this is base register
				base = token;
				state = INDEX;
				expect = "+";
				continue;
			}

			if (state == INDEX) {
				const Token* token = stream.accept(Token::NAME);

				// no index name, skip
				if (token == nullptr) {
					state = LABEL;
					continue;
				}

				index = token;
				state = SCALE;
				expect = "*";
				continue;
			}

			if (state == SCALE) {
				scale = &stream.expect(Token::INT);
				state = LABEL;
				expect = "+";
				continue;
			}

			if (state == LABEL) {
				const Token* token = stream.accept(Token::REFERENCE);

				// no reference, skip
				if (token == nullptr) {
					state = OFFSET;
					continue;
				}

				label = token;
				state = OFFSET;
				expect = "+";
				continue;
			}

			if (state == OFFSET) {
				const Token* token = stream.accept(Token::INT);

				// no offset, skip
				if (token == nullptr) {
					break;
				}

				int64_t value = token->as_int();

				switch (op) {
					case '+': offset += value; break;
					case '-': offset -= value; break;
					case '*': offset *= value; break;
					case '/': offset /= value; break;
					case '|': offset |= value; break;
					case '&': offset &= value; break;
					case '^': offset ^= value; break;
					case '%': offset %= value; break;
					default: throw std::runtime_error {"Unknown operator '" + std::string(1, op) + "'"};
				}

				if (!stream.empty()) {
					op = stream.expect(Token::OPERATOR).raw[0];
				}
			}

		}

		// nothing should be left in the stream
		stream.assert_empty();

		const char* name = (label == nullptr) ? nullptr : label->raw.c_str() + 1 /* skip the '@' */;
		const uint32_t scale_value = (scale == nullptr) ? 0 : scale->as_int();

		return token_to_register(base) + token_to_register(index) * scale_value + offset + name;
	}

	static Location parse_inner(TokenStream stream) {
		if (stream.accept("[")) {
			return parse_expression(stream.block("[]", "expression")).ref();
		}

		return parse_expression(stream);
	}

	static Location parse_location(TokenStream stream) {
		const Token* name = &stream.peek();
		const int size = token_to_sizing(name);

		if (size != -1) {
			stream.next(); // consume 'name' token if it was a cast
			return parse_inner(stream).cast(size);
		}

		return parse_inner(stream);
	}

	template <typename T>
	Location parse_argument(TokenStream stream) {
		static_assert(std::is_same_v<T, Location>, "x86 can only accept Location classes as arguments");
		return parse_location(stream);
	}

#	include "generated/x86.hpp"

	/*
	 * class LanguageModule
	 */

	const char* LanguageModule::name() const {
		return "x86";
	}

	FeatureSet LanguageModule::features() const {
		return {};
	}

	void LanguageModule::parse(ErrorHandler& reporter, TokenStream stream, SegmentedBuffer& buffer) const {
		BufferWriter writer {buffer};

		if (try_parse_instruction(stream, writer)) {
			return;
		}

		Module::parse(reporter, stream, buffer);
	}

}
