#include "emitter.hpp"

#include "writer.hpp"

using namespace tasml;

namespace asmio::x86 {

	// forward defs
	Location parseExpression(TokenStream stream);
	Location parseLocation(ErrorHandler& reporter, TokenStream stream, bool& write);
	bool parseDataDefinition(ErrorHandler& reporter, BufferWriter& writer, TokenStream& stream, const Token& token);
	bool parseSectionStatement(ErrorHandler& reporter, BufferWriter& writer, TokenStream& stream, const Token& token);

	template <uint32_t argc> struct Instruction {};
	template <> struct Instruction<0> { using type = void (BufferWriter::*) (); };
	template <> struct Instruction<1> { using type = void (BufferWriter::*) (Location); };
	template <> struct Instruction<2> { using type = void (BufferWriter::*) (Location, Location); };
	template <> struct Instruction<3> { using type = void (BufferWriter::*) (Location, Location, Location); };

	template <uint32_t argc>
	void parseCall(ErrorHandler& reporter, typename Instruction<argc>::type inst, BufferWriter& writer, TokenStream& stream) {
		try {
			bool write = true;

			if constexpr (argc == 0) {
				(writer.*inst)();
			}

			if constexpr (argc == 1) {
				Location arg1 = parseLocation(reporter, stream.expression("expression"), write);
				if (write) (writer.*inst)(arg1);
			}

			if constexpr (argc == 2) {
				Location arg1 = parseLocation(reporter, stream.expression("expression"), write);
				Location arg2 = parseLocation(reporter, stream.expression("expression"), write);
				if (write) (writer.*inst)(arg1, arg2);
			}

			if constexpr (argc == 3) {
				Location arg1 = parseLocation(reporter, stream.expression("expression"), write);
				Location arg2 = parseLocation(reporter, stream.expression("expression"), write);
				Location arg3 = parseLocation(reporter, stream.expression("expression"), write);
				if (write) (writer.*inst)(arg1, arg2, arg3);
			}
		}catch (std::runtime_error& error) {
			reporter.error(stream.first().line, -1, error.what());
		}
	}

	int getTypeByToken(const Token* token) {
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

	int countArgs(TokenStream stream) {
		int count = 0;

		// first arg doesn't have a comma before it
		if (!stream.empty()) {
			count = 1;
		}

		while (!stream.empty()) {
			if (stream.next().raw[0] == ',') {
				count ++;
			}
		}

		return count;
	}

	Registry getRegistryByToken(const Token* token) {
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

	Location parseExpression(TokenStream stream) {
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

		return getRegistryByToken(base) + getRegistryByToken(index) * scale_value + offset + name;
	}

	Location parseInner(ErrorHandler& reporter, TokenStream stream, bool& write) {
		if (stream.accept("[")) {
			return parseExpression(stream.block("[]", "expression")).ref();
		}

		return parseExpression(stream);
	}

	Location parseLocation(ErrorHandler& reporter, TokenStream stream, bool& write) {
		try {
			const Token* name = &stream.peek();
			const int size = getTypeByToken(name);

			if (size != -1) {
				stream.next(); // consume 'name' token if it was a cast
				return parseInner(reporter, stream, write).cast(size);
			}

			return parseInner(reporter, stream, write);
		} catch (std::runtime_error& error) {
			reporter.error(stream.first().line, -1, error.what());
			write = false;
			return {0};
		}
	}

	void parseInstruction(ErrorHandler& reporter, BufferWriter& writer, TokenStream& stream, const Token& token) {
		int argc = countArgs(stream);
		const char* name = token.raw.c_str();

		// auto generated using ingen.py
		if (argc == 0 && strcmp(name, "movsb") == 0) return parseCall<0>(reporter, &BufferWriter::put_movsb, writer, stream);
		if (argc == 0 && strcmp(name, "movsw") == 0) return parseCall<0>(reporter, &BufferWriter::put_movsw, writer, stream);
		if (argc == 0 && strcmp(name, "movsd") == 0) return parseCall<0>(reporter, &BufferWriter::put_movsd, writer, stream);
		if (argc == 0 && strcmp(name, "insb") == 0) return parseCall<0>(reporter, &BufferWriter::put_insb, writer, stream);
		if (argc == 0 && strcmp(name, "insw") == 0) return parseCall<0>(reporter, &BufferWriter::put_insw, writer, stream);
		if (argc == 0 && strcmp(name, "insd") == 0) return parseCall<0>(reporter, &BufferWriter::put_insd, writer, stream);
		if (argc == 0 && strcmp(name, "outsb") == 0) return parseCall<0>(reporter, &BufferWriter::put_outsb, writer, stream);
		if (argc == 0 && strcmp(name, "outsw") == 0) return parseCall<0>(reporter, &BufferWriter::put_outsw, writer, stream);
		if (argc == 0 && strcmp(name, "outsd") == 0) return parseCall<0>(reporter, &BufferWriter::put_outsd, writer, stream);
		if (argc == 0 && strcmp(name, "cmpsb") == 0) return parseCall<0>(reporter, &BufferWriter::put_cmpsb, writer, stream);
		if (argc == 0 && strcmp(name, "cmpsw") == 0) return parseCall<0>(reporter, &BufferWriter::put_cmpsw, writer, stream);
		if (argc == 0 && strcmp(name, "cmpsd") == 0) return parseCall<0>(reporter, &BufferWriter::put_cmpsd, writer, stream);
		if (argc == 0 && strcmp(name, "scasb") == 0) return parseCall<0>(reporter, &BufferWriter::put_scasb, writer, stream);
		if (argc == 0 && strcmp(name, "scasw") == 0) return parseCall<0>(reporter, &BufferWriter::put_scasw, writer, stream);
		if (argc == 0 && strcmp(name, "scasd") == 0) return parseCall<0>(reporter, &BufferWriter::put_scasd, writer, stream);
		if (argc == 0 && strcmp(name, "lodsb") == 0) return parseCall<0>(reporter, &BufferWriter::put_lodsb, writer, stream);
		if (argc == 0 && strcmp(name, "lodsw") == 0) return parseCall<0>(reporter, &BufferWriter::put_lodsw, writer, stream);
		if (argc == 0 && strcmp(name, "lodsd") == 0) return parseCall<0>(reporter, &BufferWriter::put_lodsd, writer, stream);
		if (argc == 0 && strcmp(name, "stosb") == 0) return parseCall<0>(reporter, &BufferWriter::put_stosb, writer, stream);
		if (argc == 0 && strcmp(name, "stosw") == 0) return parseCall<0>(reporter, &BufferWriter::put_stosw, writer, stream);
		if (argc == 0 && strcmp(name, "stosd") == 0) return parseCall<0>(reporter, &BufferWriter::put_stosd, writer, stream);
		if (argc == 2 && strcmp(name, "mov") == 0) return parseCall<2>(reporter, &BufferWriter::put_mov, writer, stream);
		if (argc == 2 && strcmp(name, "movsx") == 0) return parseCall<2>(reporter, &BufferWriter::put_movsx, writer, stream);
		if (argc == 2 && strcmp(name, "movzx") == 0) return parseCall<2>(reporter, &BufferWriter::put_movzx, writer, stream);
		if (argc == 2 && strcmp(name, "lea") == 0) return parseCall<2>(reporter, &BufferWriter::put_lea, writer, stream);
		if (argc == 2 && strcmp(name, "xchg") == 0) return parseCall<2>(reporter, &BufferWriter::put_xchg, writer, stream);
		if (argc == 1 && strcmp(name, "push") == 0) return parseCall<1>(reporter, &BufferWriter::put_push, writer, stream);
		if (argc == 1 && strcmp(name, "pop") == 0) return parseCall<1>(reporter, &BufferWriter::put_pop, writer, stream);
		if (argc == 0 && strcmp(name, "pop") == 0) return parseCall<0>(reporter, &BufferWriter::put_pop, writer, stream);
		if (argc == 1 && strcmp(name, "inc") == 0) return parseCall<1>(reporter, &BufferWriter::put_inc, writer, stream);
		if (argc == 1 && strcmp(name, "dec") == 0) return parseCall<1>(reporter, &BufferWriter::put_dec, writer, stream);
		if (argc == 1 && strcmp(name, "neg") == 0) return parseCall<1>(reporter, &BufferWriter::put_neg, writer, stream);
		if (argc == 2 && strcmp(name, "add") == 0) return parseCall<2>(reporter, &BufferWriter::put_add, writer, stream);
		if (argc == 2 && strcmp(name, "adc") == 0) return parseCall<2>(reporter, &BufferWriter::put_adc, writer, stream);
		if (argc == 2 && strcmp(name, "sub") == 0) return parseCall<2>(reporter, &BufferWriter::put_sub, writer, stream);
		if (argc == 2 && strcmp(name, "sbb") == 0) return parseCall<2>(reporter, &BufferWriter::put_sbb, writer, stream);
		if (argc == 2 && strcmp(name, "cmp") == 0) return parseCall<2>(reporter, &BufferWriter::put_cmp, writer, stream);
		if (argc == 2 && strcmp(name, "and") == 0) return parseCall<2>(reporter, &BufferWriter::put_and, writer, stream);
		if (argc == 2 && strcmp(name, "or") == 0) return parseCall<2>(reporter, &BufferWriter::put_or, writer, stream);
		if (argc == 2 && strcmp(name, "xor") == 0) return parseCall<2>(reporter, &BufferWriter::put_xor, writer, stream);
		if (argc == 2 && strcmp(name, "bt") == 0) return parseCall<2>(reporter, &BufferWriter::put_bt, writer, stream);
		if (argc == 2 && strcmp(name, "bts") == 0) return parseCall<2>(reporter, &BufferWriter::put_bts, writer, stream);
		if (argc == 2 && strcmp(name, "btr") == 0) return parseCall<2>(reporter, &BufferWriter::put_btr, writer, stream);
		if (argc == 2 && strcmp(name, "btc") == 0) return parseCall<2>(reporter, &BufferWriter::put_btc, writer, stream);
		if (argc == 2 && strcmp(name, "bsf") == 0) return parseCall<2>(reporter, &BufferWriter::put_bsf, writer, stream);
		if (argc == 2 && strcmp(name, "bsr") == 0) return parseCall<2>(reporter, &BufferWriter::put_bsr, writer, stream);
		if (argc == 1 && strcmp(name, "mul") == 0) return parseCall<1>(reporter, &BufferWriter::put_mul, writer, stream);
		if (argc == 2 && strcmp(name, "imul") == 0) return parseCall<2>(reporter, &BufferWriter::put_imul, writer, stream);
		if (argc == 3 && strcmp(name, "imul") == 0) return parseCall<3>(reporter, &BufferWriter::put_imul, writer, stream);
		if (argc == 1 && strcmp(name, "div") == 0) return parseCall<1>(reporter, &BufferWriter::put_div, writer, stream);
		if (argc == 1 && strcmp(name, "idiv") == 0) return parseCall<1>(reporter, &BufferWriter::put_idiv, writer, stream);
		if (argc == 1 && strcmp(name, "not") == 0) return parseCall<1>(reporter, &BufferWriter::put_not, writer, stream);
		if (argc == 2 && strcmp(name, "rol") == 0) return parseCall<2>(reporter, &BufferWriter::put_rol, writer, stream);
		if (argc == 2 && strcmp(name, "ror") == 0) return parseCall<2>(reporter, &BufferWriter::put_ror, writer, stream);
		if (argc == 2 && strcmp(name, "rcl") == 0) return parseCall<2>(reporter, &BufferWriter::put_rcl, writer, stream);
		if (argc == 2 && strcmp(name, "rcr") == 0) return parseCall<2>(reporter, &BufferWriter::put_rcr, writer, stream);
		if (argc == 2 && strcmp(name, "shl") == 0) return parseCall<2>(reporter, &BufferWriter::put_shl, writer, stream);
		if (argc == 2 && strcmp(name, "shr") == 0) return parseCall<2>(reporter, &BufferWriter::put_shr, writer, stream);
		if (argc == 2 && strcmp(name, "sal") == 0) return parseCall<2>(reporter, &BufferWriter::put_sal, writer, stream);
		if (argc == 2 && strcmp(name, "sar") == 0) return parseCall<2>(reporter, &BufferWriter::put_sar, writer, stream);
		if (argc == 3 && strcmp(name, "shld") == 0) return parseCall<3>(reporter, &BufferWriter::put_shld, writer, stream);
		if (argc == 3 && strcmp(name, "shrd") == 0) return parseCall<3>(reporter, &BufferWriter::put_shrd, writer, stream);
		if (argc == 1 && strcmp(name, "jmp") == 0) return parseCall<1>(reporter, &BufferWriter::put_jmp, writer, stream);
		if (argc == 1 && strcmp(name, "call") == 0) return parseCall<1>(reporter, &BufferWriter::put_call, writer, stream);
		if (argc == 1 && strcmp(name, "jo") == 0) return parseCall<1>(reporter, &BufferWriter::put_jo, writer, stream);
		if (argc == 1 && strcmp(name, "jno") == 0) return parseCall<1>(reporter, &BufferWriter::put_jno, writer, stream);
		if (argc == 1 && strcmp(name, "jb") == 0) return parseCall<1>(reporter, &BufferWriter::put_jb, writer, stream);
		if (argc == 1 && strcmp(name, "jnb") == 0) return parseCall<1>(reporter, &BufferWriter::put_jnb, writer, stream);
		if (argc == 1 && strcmp(name, "je") == 0) return parseCall<1>(reporter, &BufferWriter::put_je, writer, stream);
		if (argc == 1 && strcmp(name, "jne") == 0) return parseCall<1>(reporter, &BufferWriter::put_jne, writer, stream);
		if (argc == 1 && strcmp(name, "jbe") == 0) return parseCall<1>(reporter, &BufferWriter::put_jbe, writer, stream);
		if (argc == 1 && strcmp(name, "jnbe") == 0) return parseCall<1>(reporter, &BufferWriter::put_jnbe, writer, stream);
		if (argc == 1 && strcmp(name, "js") == 0) return parseCall<1>(reporter, &BufferWriter::put_js, writer, stream);
		if (argc == 1 && strcmp(name, "jns") == 0) return parseCall<1>(reporter, &BufferWriter::put_jns, writer, stream);
		if (argc == 1 && strcmp(name, "jp") == 0) return parseCall<1>(reporter, &BufferWriter::put_jp, writer, stream);
		if (argc == 1 && strcmp(name, "jnp") == 0) return parseCall<1>(reporter, &BufferWriter::put_jnp, writer, stream);
		if (argc == 1 && strcmp(name, "jl") == 0) return parseCall<1>(reporter, &BufferWriter::put_jl, writer, stream);
		if (argc == 1 && strcmp(name, "jnl") == 0) return parseCall<1>(reporter, &BufferWriter::put_jnl, writer, stream);
		if (argc == 1 && strcmp(name, "jle") == 0) return parseCall<1>(reporter, &BufferWriter::put_jle, writer, stream);
		if (argc == 1 && strcmp(name, "jnle") == 0) return parseCall<1>(reporter, &BufferWriter::put_jnle, writer, stream);
		if (argc == 1 && strcmp(name, "jc") == 0) return parseCall<1>(reporter, &BufferWriter::put_jc, writer, stream);
		if (argc == 1 && strcmp(name, "jnc") == 0) return parseCall<1>(reporter, &BufferWriter::put_jnc, writer, stream);
		if (argc == 1 && strcmp(name, "jnae") == 0) return parseCall<1>(reporter, &BufferWriter::put_jnae, writer, stream);
		if (argc == 1 && strcmp(name, "jae") == 0) return parseCall<1>(reporter, &BufferWriter::put_jae, writer, stream);
		if (argc == 1 && strcmp(name, "jz") == 0) return parseCall<1>(reporter, &BufferWriter::put_jz, writer, stream);
		if (argc == 1 && strcmp(name, "jnz") == 0) return parseCall<1>(reporter, &BufferWriter::put_jnz, writer, stream);
		if (argc == 1 && strcmp(name, "jna") == 0) return parseCall<1>(reporter, &BufferWriter::put_jna, writer, stream);
		if (argc == 1 && strcmp(name, "ja") == 0) return parseCall<1>(reporter, &BufferWriter::put_ja, writer, stream);
		if (argc == 1 && strcmp(name, "jpe") == 0) return parseCall<1>(reporter, &BufferWriter::put_jpe, writer, stream);
		if (argc == 1 && strcmp(name, "jpo") == 0) return parseCall<1>(reporter, &BufferWriter::put_jpo, writer, stream);
		if (argc == 1 && strcmp(name, "jnge") == 0) return parseCall<1>(reporter, &BufferWriter::put_jnge, writer, stream);
		if (argc == 1 && strcmp(name, "jge") == 0) return parseCall<1>(reporter, &BufferWriter::put_jge, writer, stream);
		if (argc == 1 && strcmp(name, "jng") == 0) return parseCall<1>(reporter, &BufferWriter::put_jng, writer, stream);
		if (argc == 1 && strcmp(name, "jg") == 0) return parseCall<1>(reporter, &BufferWriter::put_jg, writer, stream);
		if (argc == 1 && strcmp(name, "jcxz") == 0) return parseCall<1>(reporter, &BufferWriter::put_jcxz, writer, stream);
		if (argc == 1 && strcmp(name, "jecxz") == 0) return parseCall<1>(reporter, &BufferWriter::put_jecxz, writer, stream);
		if (argc == 1 && strcmp(name, "loop") == 0) return parseCall<1>(reporter, &BufferWriter::put_loop, writer, stream);
		if (argc == 1 && strcmp(name, "loope") == 0) return parseCall<1>(reporter, &BufferWriter::put_loope, writer, stream);
		if (argc == 1 && strcmp(name, "loopz") == 0) return parseCall<1>(reporter, &BufferWriter::put_loopz, writer, stream);
		if (argc == 1 && strcmp(name, "loopne") == 0) return parseCall<1>(reporter, &BufferWriter::put_loopne, writer, stream);
		if (argc == 1 && strcmp(name, "loopnz") == 0) return parseCall<1>(reporter, &BufferWriter::put_loopnz, writer, stream);
		if (argc == 1 && strcmp(name, "seto") == 0) return parseCall<1>(reporter, &BufferWriter::put_seto, writer, stream);
		if (argc == 1 && strcmp(name, "setno") == 0) return parseCall<1>(reporter, &BufferWriter::put_setno, writer, stream);
		if (argc == 1 && strcmp(name, "setb") == 0) return parseCall<1>(reporter, &BufferWriter::put_setb, writer, stream);
		if (argc == 1 && strcmp(name, "setnb") == 0) return parseCall<1>(reporter, &BufferWriter::put_setnb, writer, stream);
		if (argc == 1 && strcmp(name, "sete") == 0) return parseCall<1>(reporter, &BufferWriter::put_sete, writer, stream);
		if (argc == 1 && strcmp(name, "setne") == 0) return parseCall<1>(reporter, &BufferWriter::put_setne, writer, stream);
		if (argc == 1 && strcmp(name, "setbe") == 0) return parseCall<1>(reporter, &BufferWriter::put_setbe, writer, stream);
		if (argc == 1 && strcmp(name, "setnbe") == 0) return parseCall<1>(reporter, &BufferWriter::put_setnbe, writer, stream);
		if (argc == 1 && strcmp(name, "sets") == 0) return parseCall<1>(reporter, &BufferWriter::put_sets, writer, stream);
		if (argc == 1 && strcmp(name, "setns") == 0) return parseCall<1>(reporter, &BufferWriter::put_setns, writer, stream);
		if (argc == 1 && strcmp(name, "setp") == 0) return parseCall<1>(reporter, &BufferWriter::put_setp, writer, stream);
		if (argc == 1 && strcmp(name, "setnp") == 0) return parseCall<1>(reporter, &BufferWriter::put_setnp, writer, stream);
		if (argc == 1 && strcmp(name, "setl") == 0) return parseCall<1>(reporter, &BufferWriter::put_setl, writer, stream);
		if (argc == 1 && strcmp(name, "setnl") == 0) return parseCall<1>(reporter, &BufferWriter::put_setnl, writer, stream);
		if (argc == 1 && strcmp(name, "setle") == 0) return parseCall<1>(reporter, &BufferWriter::put_setle, writer, stream);
		if (argc == 1 && strcmp(name, "setnle") == 0) return parseCall<1>(reporter, &BufferWriter::put_setnle, writer, stream);
		if (argc == 1 && strcmp(name, "setc") == 0) return parseCall<1>(reporter, &BufferWriter::put_setc, writer, stream);
		if (argc == 1 && strcmp(name, "setnc") == 0) return parseCall<1>(reporter, &BufferWriter::put_setnc, writer, stream);
		if (argc == 1 && strcmp(name, "setnae") == 0) return parseCall<1>(reporter, &BufferWriter::put_setnae, writer, stream);
		if (argc == 1 && strcmp(name, "setae") == 0) return parseCall<1>(reporter, &BufferWriter::put_setae, writer, stream);
		if (argc == 1 && strcmp(name, "setz") == 0) return parseCall<1>(reporter, &BufferWriter::put_setz, writer, stream);
		if (argc == 1 && strcmp(name, "setnz") == 0) return parseCall<1>(reporter, &BufferWriter::put_setnz, writer, stream);
		if (argc == 1 && strcmp(name, "setna") == 0) return parseCall<1>(reporter, &BufferWriter::put_setna, writer, stream);
		if (argc == 1 && strcmp(name, "seta") == 0) return parseCall<1>(reporter, &BufferWriter::put_seta, writer, stream);
		if (argc == 1 && strcmp(name, "setpe") == 0) return parseCall<1>(reporter, &BufferWriter::put_setpe, writer, stream);
		if (argc == 1 && strcmp(name, "setpo") == 0) return parseCall<1>(reporter, &BufferWriter::put_setpo, writer, stream);
		if (argc == 1 && strcmp(name, "setnge") == 0) return parseCall<1>(reporter, &BufferWriter::put_setnge, writer, stream);
		if (argc == 1 && strcmp(name, "setge") == 0) return parseCall<1>(reporter, &BufferWriter::put_setge, writer, stream);
		if (argc == 1 && strcmp(name, "setng") == 0) return parseCall<1>(reporter, &BufferWriter::put_setng, writer, stream);
		if (argc == 1 && strcmp(name, "setg") == 0) return parseCall<1>(reporter, &BufferWriter::put_setg, writer, stream);
		if (argc == 1 && strcmp(name, "int") == 0) return parseCall<1>(reporter, &BufferWriter::put_int, writer, stream);
		if (argc == 0 && strcmp(name, "into") == 0) return parseCall<0>(reporter, &BufferWriter::put_into, writer, stream);
		if (argc == 0 && strcmp(name, "iret") == 0) return parseCall<0>(reporter, &BufferWriter::put_iret, writer, stream);
		if (argc == 0 && strcmp(name, "nop") == 0) return parseCall<0>(reporter, &BufferWriter::put_nop, writer, stream);
		if (argc == 0 && strcmp(name, "hlt") == 0) return parseCall<0>(reporter, &BufferWriter::put_hlt, writer, stream);
		if (argc == 0 && strcmp(name, "wait") == 0) return parseCall<0>(reporter, &BufferWriter::put_wait, writer, stream);
		if (argc == 0 && strcmp(name, "ud2") == 0) return parseCall<0>(reporter, &BufferWriter::put_ud2, writer, stream);
		if (argc == 2 && strcmp(name, "enter") == 0) return parseCall<2>(reporter, &BufferWriter::put_enter, writer, stream);
		if (argc == 0 && strcmp(name, "leave") == 0) return parseCall<0>(reporter, &BufferWriter::put_leave, writer, stream);
		if (argc == 0 && strcmp(name, "pusha") == 0) return parseCall<0>(reporter, &BufferWriter::put_pusha, writer, stream);
		if (argc == 0 && strcmp(name, "popa") == 0) return parseCall<0>(reporter, &BufferWriter::put_popa, writer, stream);
		if (argc == 0 && strcmp(name, "pushfd") == 0) return parseCall<0>(reporter, &BufferWriter::put_pushfd, writer, stream);
		if (argc == 0 && strcmp(name, "popfd") == 0) return parseCall<0>(reporter, &BufferWriter::put_popfd, writer, stream);
		if (argc == 0 && strcmp(name, "pushf") == 0) return parseCall<0>(reporter, &BufferWriter::put_pushf, writer, stream);
		if (argc == 0 && strcmp(name, "popf") == 0) return parseCall<0>(reporter, &BufferWriter::put_popf, writer, stream);
		if (argc == 0 && strcmp(name, "clc") == 0) return parseCall<0>(reporter, &BufferWriter::put_clc, writer, stream);
		if (argc == 0 && strcmp(name, "stc") == 0) return parseCall<0>(reporter, &BufferWriter::put_stc, writer, stream);
		if (argc == 0 && strcmp(name, "cmc") == 0) return parseCall<0>(reporter, &BufferWriter::put_cmc, writer, stream);
		if (argc == 0 && strcmp(name, "cld") == 0) return parseCall<0>(reporter, &BufferWriter::put_cld, writer, stream);
		if (argc == 0 && strcmp(name, "std") == 0) return parseCall<0>(reporter, &BufferWriter::put_std, writer, stream);
		if (argc == 0 && strcmp(name, "cli") == 0) return parseCall<0>(reporter, &BufferWriter::put_cli, writer, stream);
		if (argc == 0 && strcmp(name, "sti") == 0) return parseCall<0>(reporter, &BufferWriter::put_sti, writer, stream);
		if (argc == 1 && strcmp(name, "scf") == 0) return parseCall<1>(reporter, &BufferWriter::put_scf, writer, stream);
		if (argc == 1 && strcmp(name, "sdf") == 0) return parseCall<1>(reporter, &BufferWriter::put_sdf, writer, stream);
		if (argc == 1 && strcmp(name, "sif") == 0) return parseCall<1>(reporter, &BufferWriter::put_sif, writer, stream);
		if (argc == 0 && strcmp(name, "sahf") == 0) return parseCall<0>(reporter, &BufferWriter::put_sahf, writer, stream);
		if (argc == 0 && strcmp(name, "lahf") == 0) return parseCall<0>(reporter, &BufferWriter::put_lahf, writer, stream);
		if (argc == 0 && strcmp(name, "aaa") == 0) return parseCall<0>(reporter, &BufferWriter::put_aaa, writer, stream);
		if (argc == 0 && strcmp(name, "daa") == 0) return parseCall<0>(reporter, &BufferWriter::put_daa, writer, stream);
		if (argc == 0 && strcmp(name, "aas") == 0) return parseCall<0>(reporter, &BufferWriter::put_aas, writer, stream);
		if (argc == 0 && strcmp(name, "das") == 0) return parseCall<0>(reporter, &BufferWriter::put_das, writer, stream);
		if (argc == 0 && strcmp(name, "cbw") == 0) return parseCall<0>(reporter, &BufferWriter::put_cbw, writer, stream);
		if (argc == 0 && strcmp(name, "cwd") == 0) return parseCall<0>(reporter, &BufferWriter::put_cwd, writer, stream);
		if (argc == 0 && strcmp(name, "xlat") == 0) return parseCall<0>(reporter, &BufferWriter::put_xlat, writer, stream);
		if (argc == 2 && strcmp(name, "in") == 0) return parseCall<2>(reporter, &BufferWriter::put_in, writer, stream);
		if (argc == 2 && strcmp(name, "out") == 0) return parseCall<2>(reporter, &BufferWriter::put_out, writer, stream);
		if (argc == 2 && strcmp(name, "test") == 0) return parseCall<2>(reporter, &BufferWriter::put_test, writer, stream);
		if (argc == 1 && strcmp(name, "test") == 0) return parseCall<1>(reporter, &BufferWriter::put_test, writer, stream);
		if (argc == 0 && strcmp(name, "ret") == 0) return parseCall<0>(reporter, &BufferWriter::put_ret, writer, stream);
		if (argc == 1 && strcmp(name, "ret") == 0) return parseCall<1>(reporter, &BufferWriter::put_ret, writer, stream);
		if (argc == 2 && strcmp(name, "xadd") == 0) return parseCall<2>(reporter, &BufferWriter::put_xadd, writer, stream);
		if (argc == 1 && strcmp(name, "bswap") == 0) return parseCall<1>(reporter, &BufferWriter::put_bswap, writer, stream);
		if (argc == 0 && strcmp(name, "invd") == 0) return parseCall<0>(reporter, &BufferWriter::put_invd, writer, stream);
		if (argc == 0 && strcmp(name, "wbinvd") == 0) return parseCall<0>(reporter, &BufferWriter::put_wbinvd, writer, stream);
		if (argc == 2 && strcmp(name, "cmpxchg") == 0) return parseCall<2>(reporter, &BufferWriter::put_cmpxchg, writer, stream);
		if (argc == 0 && strcmp(name, "cqo") == 0) return parseCall<0>(reporter, &BufferWriter::put_cqo, writer, stream);
		if (argc == 0 && strcmp(name, "swapgs") == 0) return parseCall<0>(reporter, &BufferWriter::put_swapgs, writer, stream);
		if (argc == 0 && strcmp(name, "rdmsr") == 0) return parseCall<0>(reporter, &BufferWriter::put_rdmsr, writer, stream);
		if (argc == 0 && strcmp(name, "wrmsr") == 0) return parseCall<0>(reporter, &BufferWriter::put_wrmsr, writer, stream);
		if (argc == 0 && strcmp(name, "syscall") == 0) return parseCall<0>(reporter, &BufferWriter::put_syscall, writer, stream);
		if (argc == 0 && strcmp(name, "sysretl") == 0) return parseCall<0>(reporter, &BufferWriter::put_sysretl, writer, stream);
		if (argc == 0 && strcmp(name, "sysretc") == 0) return parseCall<0>(reporter, &BufferWriter::put_sysretc, writer, stream);
		if (argc == 0 && strcmp(name, "fnop") == 0) return parseCall<0>(reporter, &BufferWriter::put_fnop, writer, stream);
		if (argc == 0 && strcmp(name, "finit") == 0) return parseCall<0>(reporter, &BufferWriter::put_finit, writer, stream);
		if (argc == 0 && strcmp(name, "fninit") == 0) return parseCall<0>(reporter, &BufferWriter::put_fninit, writer, stream);
		if (argc == 0 && strcmp(name, "fclex") == 0) return parseCall<0>(reporter, &BufferWriter::put_fclex, writer, stream);
		if (argc == 0 && strcmp(name, "fnclex") == 0) return parseCall<0>(reporter, &BufferWriter::put_fnclex, writer, stream);
		if (argc == 1 && strcmp(name, "fstsw") == 0) return parseCall<1>(reporter, &BufferWriter::put_fstsw, writer, stream);
		if (argc == 1 && strcmp(name, "fnstsw") == 0) return parseCall<1>(reporter, &BufferWriter::put_fnstsw, writer, stream);
		if (argc == 1 && strcmp(name, "fstcw") == 0) return parseCall<1>(reporter, &BufferWriter::put_fstcw, writer, stream);
		if (argc == 1 && strcmp(name, "fnstcw") == 0) return parseCall<1>(reporter, &BufferWriter::put_fnstcw, writer, stream);
		if (argc == 0 && strcmp(name, "fld1") == 0) return parseCall<0>(reporter, &BufferWriter::put_fld1, writer, stream);
		if (argc == 0 && strcmp(name, "fld0") == 0) return parseCall<0>(reporter, &BufferWriter::put_fld0, writer, stream);
		if (argc == 0 && strcmp(name, "fldpi") == 0) return parseCall<0>(reporter, &BufferWriter::put_fldpi, writer, stream);
		if (argc == 0 && strcmp(name, "fldl2t") == 0) return parseCall<0>(reporter, &BufferWriter::put_fldl2t, writer, stream);
		if (argc == 0 && strcmp(name, "fldl2e") == 0) return parseCall<0>(reporter, &BufferWriter::put_fldl2e, writer, stream);
		if (argc == 0 && strcmp(name, "fldlt2") == 0) return parseCall<0>(reporter, &BufferWriter::put_fldlt2, writer, stream);
		if (argc == 0 && strcmp(name, "fldle2") == 0) return parseCall<0>(reporter, &BufferWriter::put_fldle2, writer, stream);
		if (argc == 1 && strcmp(name, "fldcw") == 0) return parseCall<1>(reporter, &BufferWriter::put_fldcw, writer, stream);
		if (argc == 0 && strcmp(name, "f2xm1") == 0) return parseCall<0>(reporter, &BufferWriter::put_f2xm1, writer, stream);
		if (argc == 0 && strcmp(name, "fabs") == 0) return parseCall<0>(reporter, &BufferWriter::put_fabs, writer, stream);
		if (argc == 0 && strcmp(name, "fchs") == 0) return parseCall<0>(reporter, &BufferWriter::put_fchs, writer, stream);
		if (argc == 0 && strcmp(name, "fcos") == 0) return parseCall<0>(reporter, &BufferWriter::put_fcos, writer, stream);
		if (argc == 0 && strcmp(name, "fsin") == 0) return parseCall<0>(reporter, &BufferWriter::put_fsin, writer, stream);
		if (argc == 0 && strcmp(name, "fsincos") == 0) return parseCall<0>(reporter, &BufferWriter::put_fsincos, writer, stream);
		if (argc == 0 && strcmp(name, "fdecstp") == 0) return parseCall<0>(reporter, &BufferWriter::put_fdecstp, writer, stream);
		if (argc == 0 && strcmp(name, "fincstp") == 0) return parseCall<0>(reporter, &BufferWriter::put_fincstp, writer, stream);
		if (argc == 0 && strcmp(name, "fpatan") == 0) return parseCall<0>(reporter, &BufferWriter::put_fpatan, writer, stream);
		if (argc == 0 && strcmp(name, "fprem") == 0) return parseCall<0>(reporter, &BufferWriter::put_fprem, writer, stream);
		if (argc == 0 && strcmp(name, "fprem1") == 0) return parseCall<0>(reporter, &BufferWriter::put_fprem1, writer, stream);
		if (argc == 0 && strcmp(name, "fptan") == 0) return parseCall<0>(reporter, &BufferWriter::put_fptan, writer, stream);
		if (argc == 0 && strcmp(name, "frndint") == 0) return parseCall<0>(reporter, &BufferWriter::put_frndint, writer, stream);
		if (argc == 0 && strcmp(name, "fscale") == 0) return parseCall<0>(reporter, &BufferWriter::put_fscale, writer, stream);
		if (argc == 0 && strcmp(name, "fsqrt") == 0) return parseCall<0>(reporter, &BufferWriter::put_fsqrt, writer, stream);
		if (argc == 1 && strcmp(name, "fld") == 0) return parseCall<1>(reporter, &BufferWriter::put_fld, writer, stream);
		if (argc == 1 && strcmp(name, "fild") == 0) return parseCall<1>(reporter, &BufferWriter::put_fild, writer, stream);
		if (argc == 1 && strcmp(name, "fst") == 0) return parseCall<1>(reporter, &BufferWriter::put_fst, writer, stream);
		if (argc == 1 && strcmp(name, "fstp") == 0) return parseCall<1>(reporter, &BufferWriter::put_fstp, writer, stream);
		if (argc == 1 && strcmp(name, "fist") == 0) return parseCall<1>(reporter, &BufferWriter::put_fist, writer, stream);
		if (argc == 1 && strcmp(name, "fistp") == 0) return parseCall<1>(reporter, &BufferWriter::put_fistp, writer, stream);
		if (argc == 1 && strcmp(name, "fisttp") == 0) return parseCall<1>(reporter, &BufferWriter::put_fisttp, writer, stream);
		if (argc == 1 && strcmp(name, "ffree") == 0) return parseCall<1>(reporter, &BufferWriter::put_ffree, writer, stream);
		if (argc == 1 && strcmp(name, "fcmovb") == 0) return parseCall<1>(reporter, &BufferWriter::put_fcmovb, writer, stream);
		if (argc == 1 && strcmp(name, "fcmove") == 0) return parseCall<1>(reporter, &BufferWriter::put_fcmove, writer, stream);
		if (argc == 1 && strcmp(name, "fcmovbe") == 0) return parseCall<1>(reporter, &BufferWriter::put_fcmovbe, writer, stream);
		if (argc == 1 && strcmp(name, "fcmovu") == 0) return parseCall<1>(reporter, &BufferWriter::put_fcmovu, writer, stream);
		if (argc == 1 && strcmp(name, "fcmovnb") == 0) return parseCall<1>(reporter, &BufferWriter::put_fcmovnb, writer, stream);
		if (argc == 1 && strcmp(name, "fcmovne") == 0) return parseCall<1>(reporter, &BufferWriter::put_fcmovne, writer, stream);
		if (argc == 1 && strcmp(name, "fcmovnbe") == 0) return parseCall<1>(reporter, &BufferWriter::put_fcmovnbe, writer, stream);
		if (argc == 1 && strcmp(name, "fcmovnu") == 0) return parseCall<1>(reporter, &BufferWriter::put_fcmovnu, writer, stream);
		if (argc == 1 && strcmp(name, "fcom") == 0) return parseCall<1>(reporter, &BufferWriter::put_fcom, writer, stream);
		if (argc == 1 && strcmp(name, "fcomp") == 0) return parseCall<1>(reporter, &BufferWriter::put_fcomp, writer, stream);
		if (argc == 0 && strcmp(name, "fcompp") == 0) return parseCall<0>(reporter, &BufferWriter::put_fcompp, writer, stream);
		if (argc == 1 && strcmp(name, "ficom") == 0) return parseCall<1>(reporter, &BufferWriter::put_ficom, writer, stream);
		if (argc == 1 && strcmp(name, "ficomp") == 0) return parseCall<1>(reporter, &BufferWriter::put_ficomp, writer, stream);
		if (argc == 1 && strcmp(name, "fcomi") == 0) return parseCall<1>(reporter, &BufferWriter::put_fcomi, writer, stream);
		if (argc == 1 && strcmp(name, "fcomip") == 0) return parseCall<1>(reporter, &BufferWriter::put_fcomip, writer, stream);
		if (argc == 1 && strcmp(name, "fucomi") == 0) return parseCall<1>(reporter, &BufferWriter::put_fucomi, writer, stream);
		if (argc == 1 && strcmp(name, "fucomip") == 0) return parseCall<1>(reporter, &BufferWriter::put_fucomip, writer, stream);
		if (argc == 1 && strcmp(name, "fmul") == 0) return parseCall<1>(reporter, &BufferWriter::put_fmul, writer, stream);
		if (argc == 1 && strcmp(name, "fimul") == 0) return parseCall<1>(reporter, &BufferWriter::put_fimul, writer, stream);
		if (argc == 2 && strcmp(name, "fmul") == 0) return parseCall<2>(reporter, &BufferWriter::put_fmul, writer, stream);
		if (argc == 1 && strcmp(name, "fmulp") == 0) return parseCall<1>(reporter, &BufferWriter::put_fmulp, writer, stream);
		if (argc == 1 && strcmp(name, "fadd") == 0) return parseCall<1>(reporter, &BufferWriter::put_fadd, writer, stream);
		if (argc == 1 && strcmp(name, "fiadd") == 0) return parseCall<1>(reporter, &BufferWriter::put_fiadd, writer, stream);
		if (argc == 2 && strcmp(name, "fadd") == 0) return parseCall<2>(reporter, &BufferWriter::put_fadd, writer, stream);
		if (argc == 1 && strcmp(name, "faddp") == 0) return parseCall<1>(reporter, &BufferWriter::put_faddp, writer, stream);
		if (argc == 1 && strcmp(name, "fdiv") == 0) return parseCall<1>(reporter, &BufferWriter::put_fdiv, writer, stream);
		if (argc == 1 && strcmp(name, "fidiv") == 0) return parseCall<1>(reporter, &BufferWriter::put_fidiv, writer, stream);
		if (argc == 2 && strcmp(name, "fdiv") == 0) return parseCall<2>(reporter, &BufferWriter::put_fdiv, writer, stream);
		if (argc == 1 && strcmp(name, "fdivp") == 0) return parseCall<1>(reporter, &BufferWriter::put_fdivp, writer, stream);
		if (argc == 1 && strcmp(name, "fdivr") == 0) return parseCall<1>(reporter, &BufferWriter::put_fdivr, writer, stream);
		if (argc == 1 && strcmp(name, "fidivr") == 0) return parseCall<1>(reporter, &BufferWriter::put_fidivr, writer, stream);
		if (argc == 2 && strcmp(name, "fdivr") == 0) return parseCall<2>(reporter, &BufferWriter::put_fdivr, writer, stream);
		if (argc == 1 && strcmp(name, "fdivrp") == 0) return parseCall<1>(reporter, &BufferWriter::put_fdivrp, writer, stream);

		if (parseDataDefinition(reporter, writer, stream, token)) {
			return;
		}

		if (parseSectionStatement(reporter, writer, stream, token)) {
			return;
		}

		// TODO: add rep/repe/repz/repne/repnz prefix support

		throw std::runtime_error {"Unknown " + std::to_string(argc) + " argument mnemonic '" + std::string {name} + "'"};
	}

	bool parseSectionStatement(ErrorHandler& reporter, BufferWriter& writer, TokenStream& stream, const Token& token) {

		if (util::to_lower(token.raw) != "section") {
			return false;
		}

		auto mode = util::to_lower(stream.expect(Token::NAME).raw);
		uint32_t flags = 0;

		for (char c : mode) {
			if (c == 'r') flags |= BufferSegment::R;
			else if (c == 'w') flags |= BufferSegment::W;
			else if (c == 'x') flags |= BufferSegment::X;
			else throw std::runtime_error {"Unknown section flag '" + std::to_string(c) + "'"};
		}

		writer.section(flags);
		return true;

	}

	bool parseDataDefinition(ErrorHandler& reporter, BufferWriter& writer, TokenStream& stream, const Token& token) {
		int size = getTypeByToken(&token);

		if (size == -1) {
			return false;
		}

		while (!stream.empty()) {
			TokenStream arg = stream.expression("argument");
			const Token& token = arg.next();
			arg.assert_empty();

			if (token.type == Token::INT) {
				int64_t value = token.as_int();

				if (size <= (int) sizeof(value)) {
					writer.put_data(size, &value);
				} else {
					void* data = malloc(size);
					memcpy(data, &value, sizeof(value));
					writer.put_data(size, data);
					free(data);
				}

				continue;
			}

			if (token.type == Token::FLOAT) {
				long double value80 = token.as_float();

				if (size == DWORD) {
					float value32 = (float) value80;
					writer.put_data(DWORD, &value32);
					continue;
				}

				if (size == QWORD) {
					double value64 = (double) value80;
					writer.put_data(QWORD, &value64);
					continue;
				}

				if (size == TWORD) {
					writer.put_data(TWORD, &value80);
					continue;
				}

				throw std::runtime_error {"Unable to encode floating point value into the specified size"};
            }

			if (token.type == Token::STRING) {
				std::string parsed = token.as_string();
				for (char chr : parsed) {

					if (size == BYTE) {
						writer.put_byte(chr);
					} else {
						void* data = malloc(size);
						memcpy(data, &chr, 1);
						writer.put_data(size, data);
						free(data);
					}
				}

				continue;
			}

			throw std::runtime_error {"Unexpected token '" + token.quoted() + "', expected string, integer or float"};
		}

		return true;
	}

	void parseStatement(ErrorHandler& reporter, BufferWriter& writer, TokenStream stream) {
		try {
			const Token &token = stream.next();

			if (token.type == Token::LABEL) {
				writer.label(token.as_label());
				return;
			}

			if (token.type == Token::NAME) {
				parseInstruction(reporter, writer, stream, token);

				if (!stream.empty()) {
					stream.expect(";");
				}

				return;
			}

			throw std::runtime_error {"Unexpected token " + token.quoted() + ", expected label or name"};
		} catch (std::runtime_error& error) {
			reporter.error(stream.first().line, -1, error.what());
		}
	}

	void parseBlock(ErrorHandler& reporter, BufferWriter& writer, TokenStream& stream) {
		while (!stream.empty()) {
			parseStatement(reporter, writer, stream.statement("statement"));
		}
	}

}