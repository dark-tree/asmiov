#pragma once

#include "external.hpp"
#include "segmented.hpp"
#include "label.hpp"

#define TRANSIENT_RETURN(T, format) {volatile T tmp; asm("" : format (tmp)); return tmp;}
#define CALL_POINTER(offset, type, arg) reinterpret_cast<type (*)(decltype(arg))>(buffer + offset)(arg);
#define TAIL_CALL(offset, type, format, arg) CALL_POINTER(offset, type, arg); TRANSIENT_RETURN(type, format)

namespace asmio {

	template <typename T>
	concept trivially_copyable = std::is_trivially_copyable_v<T>;

	class ExecutableBuffer {

		private:

			LabelMap<size_t> labels;
			uint8_t* buffer = nullptr;
			size_t length = 0;

		public:

			ExecutableBuffer() = default;
			explicit ExecutableBuffer(size_t total);

			ExecutableBuffer(ExecutableBuffer&& other) noexcept;
			explicit ExecutableBuffer(const ExecutableBuffer& other);
			~ExecutableBuffer();

			ExecutableBuffer& operator =(ExecutableBuffer&& other) noexcept;

			/// Copy data from segmented buffer and configure memory protection
			void bake(SegmentedBuffer& segmented);

			/// Get the base address of this buffer
			uint8_t* address() const;

			/// Get the address of a specific label
			uint8_t* address(Label label) const;

			/// get the total size, in bytes
			size_t size() const;

		public:

			template <std::integral R, trivially_copyable... Args>
			R scall(const Label& label, Args... args) {
				const uint64_t offset = labels.at(label);

				// unpack parameter pack
				constexpr uint64_t bytes = (sizeof(args) + ...);
				constexpr size_t sizes[] = { sizeof(args)... };
				const void* raws[] = { &args... };

				void* params = bytes > 0 ? alloca(bytes) : nullptr;
				auto* next = static_cast<uint8_t*>(params);

				// copy parameter pack as raw value into the buffer
				for (size_t i = 0; i < sizeof...(args); i ++) {
					const size_t element = sizes[i];

					memcpy(next, raws[i], element);
					next += element;
				}

				TAIL_CALL(offset, R, "=r", params);
			}

			/*
			 * Offset
			 */

			void call(uint32_t offset = 0) {
				CALL_POINTER(offset, void, nullptr);
			}

			uint64_t call_u64(uint32_t offset = 0) {
				TAIL_CALL(offset, uint64_t, "=r", nullptr);
			}

			int64_t call_i64(uint32_t offset = 0) {
				TAIL_CALL(offset, int64_t, "=r", nullptr);
			}

			uint32_t call_u32(uint32_t offset = 0) {
				TAIL_CALL(offset, uint32_t, "=r", nullptr);
			}

			int32_t call_i32(uint32_t offset = 0) {
				TAIL_CALL(offset, int32_t, "=r", nullptr);
			}

			float call_f32(uint32_t offset = 0) {
				TAIL_CALL(offset, float, "=t", nullptr);
			}

			/*
			 * Labels
			 */

			void call(const Label& label) {
				call(labels.at(label));
			}

			uint64_t call_u64(const Label& label) {
				return call_u64(labels.at(label));
			}

			int64_t call_i64(const Label& label) {
				return call_i64(labels.at(label));
			}

			uint32_t call_u32(const Label& label) {
				return call_u32(labels.at(label));
			}

			int32_t call_i32(const Label& label) {
				return call_i32(labels.at(label));
			}

			float call_f32(const Label& label) {
				return call_f32(labels.at(label));
			}

	};

	/// Create an ExecutableBuffer given a SegmentedBuffer
	ExecutableBuffer to_executable(SegmentedBuffer& segmented);

}
