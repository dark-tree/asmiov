#pragma once
#include <out/buffer/segmented.hpp>
#include <out/elf/buffer.hpp>

#include "error.hpp"
#include "stream.hpp"

namespace tasml {

	void assemble(ErrorHandler& reporter, TokenStream& stream, asmio::SegmentedBuffer& buffer);

	asmio::SegmentedBuffer assemble(ErrorHandler& reporter, const std::string& source);

}
