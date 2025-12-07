#include "module.hpp"

#include <out/buffer/sizes.hpp>
#include <out/buffer/writer.hpp>
#include <tasml/error.hpp>
#include <tasml/stream.hpp>

namespace asmio {

	using namespace tasml;

	template <typename T>
	static void encode_slot(BasicBufferWriter& writer, T value, int slot) {

		const int size = sizeof(T);
		writer.put_data(std::min(slot, size), &value);

		// we can only encode directly up to T size,
		// this is how many zero bytes will be added after
		// the number to reach the desired slot size
		writer.put_space(size < slot ? slot - size : 0);
	}

	static void encode_single(TokenStream& stream, BasicBufferWriter& writer, int size) {

		if (const Token* token = stream.accept(Token::INT)) {
			encode_slot(writer, token->as_int(), size);
			return;
		}

		if (const Token* token = stream.accept(Token::FLOAT)) {
			if (size == DWORD) encode_slot<float>(writer, token->as_float(), size);
			else if (size == QWORD) encode_slot<double>(writer, token->as_float(), size);
			else if (size == TWORD) encode_slot<long double>(writer, token->as_float(), size);
			else throw std::runtime_error {"Unsupported float size!"};

			return;
		}

		if (const Token* token = stream.accept(Token::STRING)) {
			for (char c : token->as_string()) {
				encode_slot(writer, c, size);
			}
		}

	}

	static void encode_args(TokenStream& stream, BasicBufferWriter& writer, int size) {
		while (true) {

			encode_single(stream, writer, size);

			if (stream.accept(",")) {
				continue;
			}

			break;
		}

		stream.terminal();
	}

	/*
	 * class Module
	 */

	Module::~Module() = default;

	const char* Module::name() const {
		return base_module;
	}

	FeatureSet Module::features() const {
		return {};
	}

	void Module::parse(ErrorHandler& reporter, TokenStream stream, SegmentedBuffer& buffer) const {

		BasicBufferWriter writer {buffer};

		/*
		 * Label Definitions
		 */

		if (const Token* token = stream.accept(Token::LABEL)) {
			stream.terminal();
			writer.label(token->as_label());
			return;
		}

		/*
		 * Section statements
		 */

		if (stream.accept("section")) {
			auto mode = util::to_lower(stream.expect(Token::NAME).raw);
			uint32_t flags = 0;
			std::string name;

			for (char c : mode) {
				if (c == 'r') flags |= BufferSegment::R;
				else if (c == 'w') flags |= BufferSegment::W;
				else if (c == 'x') flags |= BufferSegment::X;
				else throw std::runtime_error {"Unknown section flag '" + std::to_string(c) + "' in section statement"};
			}

			if (const Token* token = stream.accept(Token::STRING)) {
				name = token->as_string();
			}

			stream.terminal();
			writer.section(flags, name);
			return;
		}

		/*
		 * Symbol export
		 */

		if (stream.accept("export")) {

			ExportSymbol::Type type = ExportSymbol::PUBLIC;

			if (stream.accept("public")) {
				type = ExportSymbol::PUBLIC;
			} else if (stream.accept("private")) {
				type = ExportSymbol::PRIVATE;
			} else if (stream.accept("weak")) {
				type = ExportSymbol::WEAK;
			}

			if (const Token* token = stream.accept(Token::LABEL)) {
				Label label = token->as_label();

				writer.label(label);
				writer.export_symbol(label, type);
				return;
			}

			auto name = stream.expect(Token::REFERENCE).as_label();

			stream.terminal();
			writer.export_symbol(name, type);
			return;

		}

		/*
		 * Embed statement
		 */

		if (stream.accept("embed")) {
			const Token& token = stream.expect(Token::STRING);

			try {
				auto bytes = util::read_whole(token.as_string());
				buffer.insert(reinterpret_cast<uint8_t*>(bytes.data()), bytes.size());
			} catch (const std::exception& e) {
				reporter.error(token.line, token.column, e.what());
			}
		}

		/*
		 * Data statements
		 */

		if (stream.accept("d8") || stream.accept("byte")) return encode_args(stream, writer, BYTE);
		if (stream.accept("d16") || stream.accept("word")) return encode_args(stream, writer, WORD);
		if (stream.accept("d32") || stream.accept("dword")) return encode_args(stream, writer, DWORD);
		if (stream.accept("d64") || stream.accept("qword")) return encode_args(stream, writer, QWORD);
		if (stream.accept("d80") || stream.accept("tword")) return encode_args(stream, writer, TWORD);

		/*
		 * Unexpected token
		 */

		if (stream.empty()) {
			return;
		}

		const Token& token = stream.next();
		reporter.error(token.line, token.column, "Unknown statement " + token.quoted());

	}

	ElfMachine Module::machine() const {
		return ElfMachine::NONE;
	}

}
