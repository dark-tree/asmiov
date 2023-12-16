#pragma once

#include <cinttypes>
#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>

#include "util.hpp"

namespace asmio::x86 {

	class ExecutableBuffer {

		private:

			std::unordered_map<Label, size_t, Label::HashFunction> labels;
			uint8_t* buffer;

		public:

			explicit ExecutableBuffer(std::vector<uint8_t> data, const std::unordered_map<Label, size_t, Label::HashFunction>& labels)
			: labels(labels) {
				const size_t page = getpagesize();
				const size_t size = ALIGN_UP(data.size(), page);

				buffer = (uint8_t*) valloc(size);
				memcpy(buffer, data.data(), data.size());
				mprotect(buffer, size, PROT_EXEC | PROT_READ | PROT_WRITE);

				if (buffer == nullptr || errno) {
					throw std::runtime_error{"Failed to allocate an executable buffer!"};
				}

				#if DEBUG_MODE
				std::cout << "Executable buffer made, using " << size << " bytes" << std::endl;
				#endif
			}

			~ExecutableBuffer() {
				free(buffer);
			}

		public:

			int call(uint32_t offset = 0) {
				const int cs = 0x23; // don't touch, magic. refers to the 32bit code segment (?)
				return x86_switch_mode(cs, reinterpret_cast<uint32_t (*)()>(buffer + offset));
			}

			int call(Label label) {
				return call(labels.at(label));
			}

	};

}