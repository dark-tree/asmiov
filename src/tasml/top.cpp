#include "top.hpp"

#include <asm/module.hpp>

void tasml::parse_top_scope(ErrorHandler& reporter, TokenStream& stream, asmio::SegmentedBuffer& buffer) {

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

		module->parse(reporter, stream, buffer);

	}

}
