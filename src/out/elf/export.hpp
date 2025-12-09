#pragma once

#include <out/buffer/segmented.hpp>
#include "elf.hpp"

namespace asmio {

	ElfFile to_elf(SegmentedBuffer& segmented, const Label& entry, uint64_t address = DEFAULT_ELF_MOUNT, const Linkage::Handler& handler = nullptr);

}

