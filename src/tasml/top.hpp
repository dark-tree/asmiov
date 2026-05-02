#pragma once
#include <asmio/program/segmented.hpp>
#include <asmio/elf/object.hpp>

#include "error.hpp"
#include "stream.hpp"

namespace tasml {

	void assemble(ErrorHandler& reporter, TokenStream& stream, asmio::SegmentedBuffer& buffer);

	asmio::SegmentedBuffer assemble(ErrorHandler& reporter, const std::string& source);

	/// This is used as a helper by the tests, assemble and print errors
	asmio::SegmentedBuffer assemble(const char* unit, const std::string& source);

}
