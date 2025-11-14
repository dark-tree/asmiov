#pragma once
#include <asm/module.hpp>
#include <asm/util.hpp>

namespace asmio::arm {

	struct LanguageModule : Module {

		const char* name() const override;
		FeatureSet features() const override;
		void parse(tasml::ErrorHandler& reporter, tasml::TokenStream stream, SegmentedBuffer& buffer) const override;
		ElfMachine machine() const override;

	};

	REGISTER_MODULE(LanguageModule);

}
