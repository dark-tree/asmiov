#pragma once

#include "util.hpp"
#include "external.hpp"
#include "segmented.hpp"
#include "label.hpp"

/// FIXME REMOVE
#define RETURN_TRANSIENT(T, format) {volatile T tmp; asm("" : format (tmp)); return tmp;}

namespace asmio {

	class ExecutableBuffer {

		private:

			std::unordered_map<Label, size_t, Label::HashFunction> labels;
			uint8_t* buffer;
			size_t length;

			__attribute__((__always_inline__)) void raw_call(uint32_t offset) {
				reinterpret_cast<uint32_t (*)()>(buffer + offset)();
			}

		public:

			explicit ExecutableBuffer(size_t total) {

				// this value should already be page aligned
				length = total;

				// create a basic memory map, after this we will set the correct flags for each segment
				buffer = (uint8_t*) mmap(nullptr, length, PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

				if (buffer == nullptr) {
					throw std::runtime_error {"Failed to allocate memory map!"};
				}

			}

			void bake(SegmentedBuffer& segmented) {

				if (segmented.total() != length) {
					throw std::runtime_error {"Invalid buffer size!"};
				}

				// initialize pages
				for (const BufferSection& section : segmented.segments()) {

					uint8_t* data = buffer + section.start;
					size_t bytes = section.buffer.size();

					if (bytes == 0) {
						continue;
					}

					memcpy(data, section.buffer.data(), bytes);
					memset(data + bytes, section.padder, section.tail);
					mprotect(data, section.size(), section.get_mprot_flags());
				}

				// copy the label map
				labels = segmented.resolved_labels();

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


	inline ExecutableBuffer to_executable(SegmentedBuffer& segmented) {

		const size_t page = getpagesize();
		segmented.align(page);

		// after alignment we know how big the buffer needs to be
		ExecutableBuffer buffer {segmented.total()};

		// now that we have a buffer allocated we can link
		segmented.link((uint64_t) buffer.address());

		// finally copy data and setting to the final image
		buffer.bake(segmented);

		return buffer;
	}

}
