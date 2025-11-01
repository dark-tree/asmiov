#pragma once
#include <out/buffer/segmented.hpp>
#include "error.hpp"
#include "stream.hpp"

namespace tasml {

	void parse_top_scope(ErrorHandler& reporter, TokenStream& stream, asmio::SegmentedBuffer& buffer);

}
