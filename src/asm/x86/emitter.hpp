#pragma once

#include "writer.hpp"
#include "tasml/stream.hpp"

namespace asmio::x86 {

	Location parseExpression(int cast, bool reference, TokenStream stream);
	Location parseLocation(TokenStream stream);
	void parseInstruction(BufferWriter& writer, TokenStream& stream, const char* name);
	void parseStatement(BufferWriter& writer, TokenStream& stream);
	void parseBlock(BufferWriter& writer, TokenStream& stream);

}