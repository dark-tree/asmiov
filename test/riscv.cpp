#include <asmio/riscv/module.hpp>
#include <asmio/riscv/writer.hpp>

#include "test.hpp"
#include "vstl.hpp"

namespace test {

	using namespace asmio;
	using namespace asmio::riscv;

	TEST (riscv_check_add) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_add(X1, X7, 13);

		segmented.link(0);
		std::vector<uint8_t> s0 = {0x93, 0x80, 0xd3, 0x00};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	};

}