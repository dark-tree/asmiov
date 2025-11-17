#include "module.hpp"
#include "writer.hpp"

#include <tasml/stream.hpp>

namespace asmio::arm {

	using namespace tasml;

	template <typename T>
	T parse_argument(TokenStream stream);

	template <std::integral T>
	T parse_argument(TokenStream stream) {
		return stream.expect(Token::INT).as_int();
	}

	template <>
	Registry parse_argument(TokenStream stream) {
		const Token& token = stream.expect(Token::NAME);
		std::string raw = util::to_lower(token.raw);

		if (raw == "wzr") return WZR;
		if (raw == "xzr") return XZR;
		if (raw == "lr") return LR;
		if (raw == "sp") return SP;
		if (raw == "fp") return FP;

		if (raw[0] == 'x') {
			int index = util::parse_decimal(raw.substr(1));

			if (index >= 0 && index <= 30) {
				return X(index);
			}

			throw std::runtime_error {"Invalid register number, expected value in range [0, 30]"};
		}

		if (raw[0] == 'w') {
			int index = util::parse_decimal(raw.substr(1));

			if (index >= 0 && index <= 30) {
				return W(index);
			}

			throw std::runtime_error {"Invalid register number, expected value in range [0, 30]"};
		}

		throw std::runtime_error {"Invalid argument format, expected register"};
	}

	template <>
	Label parse_argument(TokenStream stream) {
		const Token& label = stream.expect(Token::REFERENCE);
		return {label.raw.c_str() + 1};
	}

	template <>
	Sizing parse_argument(TokenStream stream) {
		const Token& token = stream.expect(Token::NAME);
		std::string raw = util::to_lower(token.raw);

		if (raw == "ub") return Sizing::UB;
		if (raw == "uh") return Sizing::UH;
		if (raw == "uw") return Sizing::UW;
		if (raw == "ux") return Sizing::UX;

		if (raw == "sb") return Sizing::UB;
		if (raw == "sh") return Sizing::UH;
		if (raw == "sw") return Sizing::UW;
		if (raw == "sx") return Sizing::UX;

		throw std::runtime_error {"Invalid argument format, expected sizing specifier"};
	}

	template <>
	ShiftType parse_argument(TokenStream stream) {
		const Token& token = stream.expect(Token::NAME);
		std::string raw = util::to_lower(token.raw);

		if (raw == "lsl") return ShiftType::LSL;
		if (raw == "lsr") return ShiftType::LSR;
		if (raw == "asr") return ShiftType::ASR;
		if (raw == "ror") return ShiftType::ROR;

		throw std::runtime_error {"Invalid argument format, expected shift specifier"};
	}

	template <>
	Condition parse_argument(TokenStream stream) {
		const Token& token = stream.expect(Token::NAME);
		std::string raw = util::to_lower(token.raw);

		if (raw == "eq") return Condition::EQ;
		if (raw == "ne") return Condition::NE;
		if (raw == "cs") return Condition::CS;
		if (raw == "cc") return Condition::CC;
		if (raw == "mi") return Condition::MI;
		if (raw == "pl") return Condition::PL;
		if (raw == "vs") return Condition::VS;
		if (raw == "vc") return Condition::VC;
		if (raw == "hi") return Condition::HI;
		if (raw == "ls") return Condition::LS;
		if (raw == "ge") return Condition::GE;
		if (raw == "lt") return Condition::LT;
		if (raw == "gt") return Condition::GT;
		if (raw == "le") return Condition::LE;
		if (raw == "al") return Condition::AL;
		if (raw == "nv") return Condition::NV;

		throw std::runtime_error {"Invalid argument format, expected condition specifier"};
	}

	template <>
	BitPattern parse_argument(TokenStream stream) {
		if (const Token* token = stream.accept(Token::INT)) {
			return BitPattern::try_pack(token->as_int());
		}

		// TODO triplet-style bit patters and tiled integers
		throw std::runtime_error {"Invalid argument format, expected 64 bit pattern"};
	}

#	include "generated/aarch64.hpp"

	/*
	 * class LanguageModule
	 */

	const char* LanguageModule::name() const {
		return "aarch64";
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

	ElfMachine LanguageModule::machine() const {
		return ElfMachine::AARCH64;
	}

}
