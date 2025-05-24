#pragma once

#include "external.hpp"
#include "util.hpp"

namespace asmio::x86 {

	class ExecutableBuffer {

		private:

			std::unordered_map<Label, size_t, Label::HashFunction> labels;
			uint8_t* buffer;
			size_t length;

			__attribute__((__always_inline__)) void raw_call(uint32_t offset) {
				x86_call_into(reinterpret_cast<uint32_t (*)()>(buffer + offset));
			}

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
			}

			void bake(const std::vector<uint8_t>& data) {
				if (data.size() != length) {
					throw std::runtime_error{"Invalid size!"};
				}

				memcpy(buffer, data.data(), data.size());
			}

			~ExecutableBuffer() {
				free(buffer);
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