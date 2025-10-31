#pragma once
#include <asm/module.hpp>

namespace asmio::x86 {

	struct LanguageModule : Module {

		const char* name() const override;
		FeatureSet features() const override;
		virtual void parse(tasml::ErrorHandler& reporter, tasml::TokenStream& stream, SegmentedBuffer& buffer) const;

	};

}
