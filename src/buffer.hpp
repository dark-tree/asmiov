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

			void* buffer;

		public:

			explicit ExecutableBuffer(std::vector<uint8_t> data) {
				const size_t page = getpagesize();
				const size_t size = ALIGN_UP(data.size(), page);

				buffer = valloc(size);
				memcpy(buffer, data.data(), size);
				mprotect(buffer, size, PROT_EXEC | PROT_READ);

				if (buffer == nullptr || errno) {
					throw std::runtime_error{"Failed to allocate an executable buffer!"};
				}

				std::cout << "Executable buffer made, using " << size << " bytes" << std::endl;
			}

			~ExecutableBuffer() {
				free(buffer);
			}

		public:

			int call() {
				const int cs = 0x23; // don't touch, magic
				return x86_switch_mode(cs, reinterpret_cast<uint32_t (*)()>(buffer));
			}

	};

}