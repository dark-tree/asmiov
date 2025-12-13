#include "emitter.hpp"

namespace asmio {

	/*
	 * region DwarfArrayEmitter
	 */

	DwarfArrayEmitter::DwarfArrayEmitter(ChunkBuffer::Ptr& buffer, int address_bytes) {
		auto region = buffer->chunk();
		head = region->chunk();
		body = region->chunk();

		head->link<uint32_t>([=] { return region->size() - 4; });
		head->put<uint16_t>(DWARF_VERSION);
		head->put<uint8_t>(address_bytes);
		head->put<uint8_t>(0); // segment selector size, or 0
	}

}
