#include "module.hpp"
#include "writer.hpp"

#include <tasml/stream.hpp>

namespace asmio::riscv {

	using namespace tasml;

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
