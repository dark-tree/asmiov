#pragma once
#include <asm/module.hpp>
#include <asm/util.hpp>

namespace asmio::arm {

	struct LanguageModule : Module {

		const char* name() const override;
		FeatureSet features() const override;
		virtual void parse(tasml::ErrorHandler& reporter, tasml::TokenStream stream, SegmentedBuffer& buffer) const;

	};

	REGISTER_MODULE(LanguageModule);

}
