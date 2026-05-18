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

		throw std::runtime_error {"Invalid argument format, expected register"};
	}

	template <>
	Condition parse_argument(TokenStream stream) {
		const Token& token = stream.expect(Token::NAME);
		std::string raw = util::to_lower(token.raw);

		return parse_condition_enum(raw);
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
