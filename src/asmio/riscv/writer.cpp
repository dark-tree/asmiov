#include "writer.hpp"

namespace asmio::riscv {

	/*
	 * class BufferWriter
	 */

	BufferWriter::BufferWriter(SegmentedBuffer& buffer)
		: BasicBufferWriter(buffer) {
	}

}