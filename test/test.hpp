#pragma once

#include <string>
#include <asmio/util/platform.hpp>

namespace test {

	inline void dump(asmio::SegmentedBuffer& buffer) {

		if (buffer.elf_machine == asmio::ElfMachine::NONE) {
			buffer.elf_machine = asmio::ElfMachine::NATIVE;
		}

		asmio::ObjectFile baked = asmio::to_elf(buffer, asmio::Label::UNSET).bake();
		asmio::util::TempFile temp {baked};

		std::string out = asmio::call_shell("objdump --visualize-jumps -wxd -Mintel " + temp.path());
		printf("%s\n", out.c_str());

		printf("\nAuto-generated assertions:\n\n");
		printf("\tbuffer.link(0);\n");

		int i = 0;

		for (auto& segment : buffer.segments()) {
			int count = segment.buffer.size();

			if (count == 0) {
				i ++;
				continue;
			}

			count --;

			printf("\tstd::vector<uint8_t> s%d = {", i);

			for (int j = 0; j <= count; j ++) {
				printf("0x%02x", segment.buffer[j]);

				if (count != j) {
					printf(", ");
				}
			}

			printf("};\n");
			printf("\tCHECK(buffer.segments()[%d].buffer, s%d); // %s\n\n", i, i, segment.name.c_str());
			i ++;
		}

	}

}
