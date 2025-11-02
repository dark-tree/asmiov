#include "top.hpp"

#include <asm/module.hpp>
#include <out/elf/buffer.hpp>

#include "tokenizer.hpp"

namespace tasml {

	void assemble(ErrorHandler& reporter, TokenStream& stream, asmio::SegmentedBuffer& buffer) {

		using namespace asmio;

		Module* module = modules.at(Module::base_module).get();

		while (!stream.empty()) {

			if (stream.accept("language") || stream.accept("lang")) {
				const Token& name = stream.expect(Token::NAME);

				auto it = modules.find(name.raw);

				if (it == modules.end()) {
					throw std::runtime_error {"No such module '" + name.raw + "' defined!"};
				}

				module = it->second.get();
			}


			TokenStream statement = stream.statement();

			// shouldn't happen
			if (statement.empty()) {
				continue;
			}

			try {
				module->parse(reporter, statement, buffer);
			} catch (std::runtime_error& error) {
				const Token& first = statement.first();
				reporter.error(first.line, first.column, std::string(error.what()) + ", after " + first.quoted());
			}

		}

	}

	asmio::SegmentedBuffer assemble(ErrorHandler& reporter, const std::string& source) {

		// tokenize input
		std::vector<Token> tokens = tokenize(reporter, source);
		TokenStream stream {tokens};

		if (!reporter.ok()) {
			throw std::runtime_error {"Failed to tokenize input"};
		}

		// parse and assemble
		asmio::SegmentedBuffer buffer;
		assemble(reporter, stream, buffer);

		if (!reporter.ok()) {
			throw std::runtime_error {"Failed to parse input"};
		}

		return buffer;

	}

}