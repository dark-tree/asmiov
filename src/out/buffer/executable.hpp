#pragma once

#include "util.hpp"
#include "external.hpp"
#include "segmented.hpp"
#include "label.hpp"

#define RETURN_TRANSIENT(T, format) {volatile T tmp; asm("" : format (tmp)); return tmp;}

// https://stackoverflow.com/a/66249936
#if defined(__x86_64__) || defined(_M_X64)
#	define CALL_INTO_BUFFER(offset, type, format) reinterpret_cast<type (*)()>(buffer + offset)(); RETURN_TRANSIENT(type, format)
#else
#	error "Unsupported architecture!"
#endif

namespace asmio {

	class ExecutableBuffer {

		private:

			LabelMap<size_t> labels;
			uint8_t* buffer;
			size_t length;

		public:

			explicit ExecutableBuffer(size_t total);
			~ExecutableBuffer();

			/// Copy data from segmented buffer and configure memory protection
			void bake(SegmentedBuffer& segmented);

			/// Get the base address of this buffer
			uint8_t* address() const;

			/// Get the address of a specific label
			uint8_t* address(Label label) const;

			/// get the total size, in bytes
			size_t size() const;

		public:

			/*
			 * Offset
			 */

			uint64_t call_u64(uint32_t offset = 0) {
				CALL_INTO_BUFFER(offset, uint64_t, "=r");
			}

			int64_t call_i64(uint32_t offset = 0) {
				CALL_INTO_BUFFER(offset, int64_t, "=r");
			}

			uint32_t call_u32(uint32_t offset = 0) {
				CALL_INTO_BUFFER(offset, uint32_t, "=r");
			}

			int32_t call_i32(uint32_t offset = 0) {
				CALL_INTO_BUFFER(offset, int32_t, "=r");
			}

			float call_f32(uint32_t offset = 0) {
				CALL_INTO_BUFFER(offset, float, "=t");
			}

			/*
			 * Labels
			 */

			uint64_t call_u64(Label label) {
				return call_u64(labels.at(label));
			}

			int64_t call_i64(Label label) {
				return call_i64(labels.at(label));
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

	/// Create an ExecutableBuffer given a SegmentedBuffer
	ExecutableBuffer to_executable(SegmentedBuffer& segmented);

}
