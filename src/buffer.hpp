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
			size_t length;

		public:

			explicit ExecutableBuffer(size_t length, const std::unordered_map<Label, size_t, Label::HashFunction>& labels)
			: labels(labels), length(length) {
				const size_t page = getpagesize();
				const size_t size = ALIGN_UP(length, page);

				buffer = (uint8_t*) valloc(size);
				mprotect(buffer, size, PROT_EXEC | PROT_READ | PROT_WRITE);

				if (buffer == nullptr || errno) {
					throw std::runtime_error{"Failed to allocate an executable buffer!"};
				}

				#if DEBUG_MODE
				std::cout << "Executable buffer made, using " << size << " bytes" << std::endl;
				#endif
			}

			void write(const std::vector<uint8_t>& data) {
				if (data.size() != length) {
					throw std::runtime_error{"Invalid size!"};
				}

				memcpy(buffer, data.data(), data.size());
			}

			~ExecutableBuffer() {
				free(buffer);
			}

			size_t get_address() const {
				return (size_t) buffer;
			}

		public:

			uint32_t call(uint32_t offset = 0) {
				const int cs = 0x23; // don't touch, magic. refers to the 32bit code segment (?)
				return x86_switch_mode(cs, reinterpret_cast<uint32_t (*)()>(buffer + offset));
			}

			float call_float(uint32_t offset = 0) {
				const int cs = 0x23; // don't touch, magic. refers to the 32bit code segment (?)
				x86_switch_mode(cs, reinterpret_cast<uint32_t (*)()>(buffer + offset));

				// pray to the compiler gods this works
				volatile float tmp;
				asm ("" : "=t" (tmp));
				return tmp;
			}

			uint32_t call(Label label) {
				return call(labels.at(label));
			}

			float call_float(Label label) {
				return call(labels.at(label));
			}

	};

}