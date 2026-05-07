#pragma once

#include <asmio/program/writer.hpp>

namespace asmio::riscv {

	class BufferWriter : public BasicBufferWriter {

		public:

			BufferWriter(SegmentedBuffer& buffer);

	};

}