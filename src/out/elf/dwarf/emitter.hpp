#pragma once
#include <out/chunk/buffer.hpp>

namespace asmio {

	struct DwarfArrayEmitter {

		constexpr static int DWARF_VERSION = 5;

		ChunkBuffer::Ptr head;
		ChunkBuffer::Ptr body;

		DwarfArrayEmitter(ChunkBuffer::Ptr& buffer, int address_bytes);

	};

}
