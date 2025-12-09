#pragma once
#include <out/buffer/segmented.hpp>
#include <out/elf/elf.hpp>

#include "error.hpp"
#include "stream.hpp"

namespace tasml {

	void assemble(ErrorHandler& reporter, TokenStream& stream, asmio::SegmentedBuffer& buffer);

	asmio::SegmentedBuffer assemble(ErrorHandler& reporter, const std::string& source);

	/// This is used as a helper by the tests, assemble and print errors
	asmio::SegmentedBuffer assemble(const char* unit, const std::string& source);

}
