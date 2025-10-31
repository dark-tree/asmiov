#pragma once

// TODO decouple from tasml, those classes should be moved to asmio
namespace tasml {
	struct ErrorHandler;
	class TokenStream;
}

namespace asmio {

	class SegmentedBuffer;

	/// Struct describing capabilities of a particular module
	struct FeatureSet {};

	/// Modules represent architectures
	struct Module {

		virtual ~Module();

		/**
		 * Name of particular module, modules can be retrieved by this name
		 * from the module registry, this is used by TASML's language directive,
		 * to select the language to use.
		 */
		virtual const char* name() const = 0;

		/**
		 * Get a list of features supported by this modules, users of a modules
		 * should check for capabilities they require and either reject the
		 * modules or work around the limitations.
		 */
		virtual FeatureSet features() const = 0;

		/**
		 * Parse a single statement from the given token stream,
		 * and return control to the caller. When creating a module
		 * the super method must be called by the overriding one.
		 */
		virtual void parse(tasml::ErrorHandler& reporter, tasml::TokenStream& stream, SegmentedBuffer& buffer) const;

	};

}
