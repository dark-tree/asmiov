#pragma once

#include "writer.hpp"
#include "tasml/stream.hpp"
#include "tasml/error.hpp"

namespace asmio::x86 {

	Location parseExpression(int cast, bool reference, TokenStream stream);
	Location parseLocation(ErrorHandler& reporter, TokenStream stream, bool& write);
	void parseInstruction(ErrorHandler& reporter, BufferWriter& writer, TokenStream& stream, const char* name);
	void parseStatement(ErrorHandler& reporter, BufferWriter& writer, TokenStream& stream);
	void parseBlock(ErrorHandler& reporter, BufferWriter& writer, TokenStream& stream);

}