#pragma once

#include "writer.hpp"
#include "tasml/stream.hpp"
#include "tasml/error.hpp"

namespace asmio::x86 {

	void parseStatement(tasml::ErrorHandler& reporter, BufferWriter& writer, tasml::TokenStream stream);
	void parseBlock(tasml::ErrorHandler& reporter, BufferWriter& writer, tasml::TokenStream& stream);

}