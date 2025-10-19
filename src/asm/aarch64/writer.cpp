#include "writer.hpp"

namespace asmio::arm {

	/*
	 * class BufferWriter
	 */

	BufferWriter::BufferWriter(SegmentedBuffer& buffer)
		: BasicBufferWriter(buffer) {
	}

}