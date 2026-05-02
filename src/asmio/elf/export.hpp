#pragma once

#include <asmio/program/segmented.hpp>
#include "object.hpp"
#include "model.hpp"

namespace asmio {

	ElfModel to_elf(SegmentedBuffer& segmented, const Label& entry, uint64_t address = DEFAULT_ELF_MOUNT, const LinkReporter& handler = nullptr);

}

