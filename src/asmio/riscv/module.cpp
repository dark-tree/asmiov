#include "module.hpp"
#include "writer.hpp"

#include <tasml/stream.hpp>

namespace asmio::riscv {

	using namespace tasml;

	template <typename T>
	T parse_argument(TokenStream stream);

	template <std::integral T>
	T parse_argument(TokenStream stream) {
		return stream.expect(Token::INT).as_int();
	}

	template <>
	Label parse_argument(TokenStream stream) {
		const Token& label = stream.expect(Token::REFERENCE);
		return {label.raw.c_str() + 1};
	}

	template <>
	Registry parse_argument(TokenStream stream) {
		const Token& token = stream.expect(Token::NAME);
		std::string raw = util::to_lower(token.raw);

		if (raw[0] == 'x') {
			int index = util::parse_decimal(raw.substr(1));

			if (index >= 0 && index <= 31) {
				return X(index);
			}

			throw std::runtime_error {"Invalid register number, expected value in range [0, 31]"};
		}
		
		if (raw == "ra") return RA;
		if (raw == "sp") return SP;
		if (raw == "gp") return GP;
		if (raw == "tp") return TP;
		if (raw == "t0") return T0;
		if (raw == "t1") return T1;
		if (raw == "t2") return T2;
		if (raw == "s0") return S0;
		if (raw == "s1") return S1;
		if (raw == "a0") return A0;
		if (raw == "a1") return A1;
		if (raw == "a2") return A2;
		if (raw == "a3") return A3;
		if (raw == "a4") return A4;
		if (raw == "a5") return A5;
		if (raw == "a6") return A6;
		if (raw == "a7") return A7;
		if (raw == "s2") return S2;
		if (raw == "s3") return S3;
		if (raw == "s4") return S4;
		if (raw == "s5") return S5;
		if (raw == "s6") return S6;
		if (raw == "s7") return S7;
		if (raw == "s8") return S8;
		if (raw == "s9") return S9;
		if (raw == "s10") return S10;
		if (raw == "s11") return S11;
		if (raw == "t3") return T3;
		if (raw == "t4") return T4;
		if (raw == "t5") return T5;
		if (raw == "t6") return T6;

		throw std::runtime_error {"Invalid argument format, expected register"};
	}

	template <>
	Condition parse_argument(TokenStream stream) {
		const Token& token = stream.expect(Token::NAME);
		std::string raw = util::to_lower(token.raw);

		return parse_condition_enum(raw);
	}

	template <>
	Order parse_argument(TokenStream stream) {
		const Token& token = stream.expect(Token::NAME);
		std::string raw = util::to_lower(token.raw);

		return parse_order_enum(raw);
	}

	template <>
	Size parse_argument(TokenStream stream) {
		const Token& token = stream.expect(Token::NAME);
		std::string raw = util::to_lower(token.raw);

		if (raw == "dword") return DWORD;
		if (raw == "qword") return QWORD;

		throw std::runtime_error {"Invalid size"};
	}

#	include "generated/riscv.hpp"

	/*
	 * class LanguageModule
	 */

	const char* LanguageModule::name() const {
		return "riscv";
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
		return ElfMachine::RISCV;
	}

}
