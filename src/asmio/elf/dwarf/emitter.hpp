#pragma once
#include <asmio/util/chunk.hpp>

namespace asmio {

	struct DwarfArrayEmitter {

		ChunkBuffer::Ptr head;
		ChunkBuffer::Ptr body;

		DwarfArrayEmitter(ChunkBuffer::Ptr& buffer, int address_bytes);

	};

}
