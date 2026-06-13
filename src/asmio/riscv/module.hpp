#pragma once
#include <asmio/module.hpp>
#include <asmio/util.hpp>

namespace asmio::riscv {

	struct LanguageModule : Module {

		const char* name() const override;
		FeatureSet features() const override;
		void parse(tasml::ErrorHandler& reporter, tasml::TokenStream stream, SegmentedBuffer& buffer) const override;
		ElfMachine machine() const override;

	};

	REGISTER_MODULE(LanguageModule);

}
