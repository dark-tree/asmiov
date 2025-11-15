#pragma once

#include "external.hpp"
#include "segmented.hpp"
#include "label.hpp"

#define TRANSIENT_RETURN(T, format) {volatile T tmp; asm("" : format (tmp)); return tmp;}
#define CALL_POINTER(offset, type, arg) reinterpret_cast<type (*)(decltype(arg))>(buffer + offset)(arg);

#define X86_CLOBBERS_NO_ST0 "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", \
	"xmm0","xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",                       \
	"xmm8","xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",                 \
	"mm0","mm1", "mm2", "mm3", "mm4", "mm5", "mm6", "mm6",                               \
	"st(1)", "st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)"

namespace asmio {

	template <typename T>
	concept trivially_copyable = std::is_trivially_copyable_v<T>;

	template <typename T>
	concept integral_or_void = std::is_integral_v<T> || std::is_void_v<T>;

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

			template <integral_or_void R, trivially_copyable... Args>
			R scall(size_t offset, Args... args) {

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

				return CALL_POINTER(offset, R, params);
			}

			template <integral_or_void R, trivially_copyable... Args>
			R scall(const Label& label, Args... args) {
				return scall<R>(labels.at(label), args...);
			}

			/*
			 * Offset
			 */

			void call(uint32_t offset = 0) {
				CALL_POINTER(offset, void, nullptr);
			}

			uint64_t call_u64(uint32_t offset = 0) {
				return CALL_POINTER(offset, uint64_t, nullptr);
			}

			int64_t call_i64(uint32_t offset = 0) {
				return CALL_POINTER(offset, int64_t, nullptr);
			}

			uint32_t call_u32(uint32_t offset = 0) {
				return CALL_POINTER(offset, uint32_t, nullptr);
			}

			int32_t call_i32(uint32_t offset = 0) {
				return CALL_POINTER(offset, int32_t, nullptr);
			}

			float call_f32(uint64_t offset = 0) {
#if ARCH_X86
				auto function = buffer + offset;
				volatile float tmp;
				asm("call *%1" : "=t" (tmp) : "r" (function) : X86_CLOBBERS_NO_ST0);
				return tmp;
#endif

				throw std::runtime_error {"Float calls are unimplemented!"};
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
