#pragma once
#include <memory>
#include <unordered_map>
#include "util.hpp"

// TODO decouple from tasml, those classes should be moved to asmio
namespace tasml {
	struct ErrorHandler;
	class TokenStream;
}

#define REGISTER_MODULE(Type) STATIC_BLOCK { \
	auto module = std::make_unique<Type>(); \
	std::string name = module->name(); \
	modules[name] = std::move(module); \
}

namespace asmio {

	class SegmentedBuffer;

	/// Struct describing capabilities of a particular module
	struct FeatureSet {};

	/// Modules represent architectures
	struct Module {

		constexpr static const char* base_module = "base";

		virtual ~Module();

		/**
		 * Name of particular module, modules can be retrieved by this name
		 * from the module registry, this is used by TASML's language directive,
		 * to select the language to use.
		 */
		virtual const char* name() const;

		/**
		 * Get a list of features supported by this modules, users of a modules
		 * should check for capabilities they require and either reject the
		 * modules or work around the limitations.
		 */
		virtual FeatureSet features() const;

		/**
		 * Parse a single statement from the given token stream,
		 * and return control to the caller. When creating a module
		 * the super method must be called by the overriding one.
		 */
		virtual void parse(tasml::ErrorHandler& reporter, tasml::TokenStream stream, SegmentedBuffer& buffer) const;

	};

	/// module registry, to add a new language to the registry use REGISTER_MODULE(ModuleName);
	inline std::unordered_map<std::string, std::unique_ptr<Module>> modules;

	/// The base module is not very interesting but is used by default by TASML
	REGISTER_MODULE(Module);

}
