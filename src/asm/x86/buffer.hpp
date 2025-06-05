#pragma once

#include <iostream>

#include "external.hpp"
#include "util.hpp"

namespace asmio::x86 {

	class ExecutableBuffer {

		private:

			std::unordered_map<Label, size_t, Label::HashFunction> labels;
			uint8_t* buffer;
			size_t length;

			__attribute__((__always_inline__)) void raw_call(uint32_t offset) {
				reinterpret_cast<uint32_t (*)()>(buffer + offset)();
			}

		public:

			explicit ExecutableBuffer(size_t length, const std::unordered_map<Label, size_t, Label::HashFunction>& labels)
			: labels(labels), length(length) {
				const size_t page = getpagesize();
				const size_t size = ALIGN_UP(length, page);

				constexpr uint32_t flags = MAP_ANONYMOUS | MAP_PRIVATE;
				constexpr uint32_t protection = PROT_READ | PROT_WRITE | PROT_EXEC;
				buffer = (uint8_t*) mmap(nullptr, size, protection, flags, -1, 0);

				if (buffer == nullptr) {
					throw std::runtime_error{"Failed to allocate an executable buffer!"};
				}
			}

			void bake(const std::vector<uint8_t>& data) {
				if (data.size() != length) {
					throw std::runtime_error{"Invalid size!"};
				}

				memcpy(buffer, data.data(), data.size());
			}

			~ExecutableBuffer() {
				munmap(buffer, length);
			}

			uint8_t* address() const {
				return buffer;
			}

			uint8_t* address(Label label) const {
				return buffer + labels.at(label);
			}

			size_t size() const {
				return length;
			}

		public:

			void dump(uint32_t offset = 0) {
				std::cout << "./unasm.sh \"db ";
				bool first = true;

				for (int i = 0; i < offset; i++) {
					if (!first) {
						std::cout << ", ";
					}

					first = false;
					std::cout << '0' << std::setfill('0') << std::setw(2) << std::hex << ((int) buffer[i]) << "h";
				}

				std::cout << "\" \"db ";
				first = true;

				for (int i = offset; i < length; i++) {
					if (!first) {
						std::cout << ", ";
					}

					first = false;
					std::cout << '0' << std::setfill('0') << std::setw(2) << std::hex << ((int) buffer[i]) << "h";
				}

				std::cout << '"' << std::endl;
			}

			uint64_t call_u64(uint32_t offset = 0) {
				raw_call(offset);
				RETURN_TRANSIENT(uint64_t, "=r");
			}

			uint32_t call_u32(uint32_t offset = 0) {
				raw_call(offset);
				RETURN_TRANSIENT(uint32_t, "=r");
			}

			int32_t call_i32(uint32_t offset = 0) {
				raw_call(offset);
				RETURN_TRANSIENT(int32_t, "=r");
			}

			float call_f32(uint32_t offset = 0) {
				raw_call(offset);
				RETURN_TRANSIENT(float, "=t");
			}

			void dump(Label label) {
				dump(labels.at(label));
			}

			uint32_t call_u64(Label label) {
				return call_u64(labels.at(label));
			}

			uint32_t call_u32(Label label) {
				return call_u32(labels.at(label));
			}

			int32_t call_i32(Label label) {
				return call_i32(labels.at(label));
			}

			float call_f32(Label label) {
				return call_f32(labels.at(label));
			}

	};

}