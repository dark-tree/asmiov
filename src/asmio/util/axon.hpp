#pragma once

#include <asmio/aarch64/writer.hpp>
#include <asmio/program/executable.hpp>
#include <asmio/program/segmented.hpp>
#include <asmio/riscv/writer.hpp>
#include <asmio/x86/writer.hpp>

#include "trait.hpp"

namespace asmio::axon {

	//                                       external raw pointer call (Args...)
	//                                                    |
	//                                                   \|/
	//    +-----------------------+              +-----buffer-----+
	//    | bind_native(          | =generated=> | asm trampoline | --------> set_call_context()
	//    |  forward_lambda_call, |              | ContextWrapper |            |
	//    |  std::function        |              | buffer size    |            | thread_local
	//    | ) -> calls emit_*()   |              +----------------+            | ContextWrapper*
	//    +-----------------------+                      |                     | context_side_channel
	//        /|\    | raw ptr                          \|/                    |
	//         |    \|/                  +-------------------------------+    \|/
	//     +-----------------------+     | forward_with_context(Args...) | <- get_call_context()
	//     | coerce(std::function) |     +-------------------------------+
	//     +-----------------------+            |
	//        /|\    | raw ptr                  |   If we used decay(lambda)
	//         |    \|/                        \|/  context *is* the lambda (with capture)
	//     +---------------+             +--------------------------------------+
	//     | decay(lambda) |             | forward_lambda_call(lambda, Args...) |
	//     +---------------+             +--------------------------------------+
	//            |                             |
	//           \|/                           \|/
	//     raw function ptr                lambda::operator(Args...)

	namespace detail {

		// Used at the trampoline generation level to wrap the given function and context, this way to
		// common trampoline function can call the given callback with the given context (and any
		// extra arguments). This architecture allows us to handle adding the extra Context argument in
		// C++, avoiding all issues related from screwing with the calling convention too much.
		template <typename R, typename T, typename... Args>
		struct ContextWrapper {
			R (* function) (const T*, Args...);
			T context;
		};

		// Used for a very small window during each call to the raw function to pass the ContextWrapper
		// pointer from generated assembly trampoline to the C++ trampoline (forward_with_context).
		inline thread_local void* context_side_channel;

		// Called from the generated assembly trampoline
		template <typename T>
		void set_call_context(T* wrapped) noexcept {
			context_side_channel = wrapped;
		}

		// Called from the C++ trampoline (forward_with_context)
		template <typename T>
		T* get_call_context() noexcept {
			return static_cast<T*>(context_side_channel);
		}

		// The C++ trampoline called from generated assembly
		template <typename W, typename... Args>
		auto forward_with_context(Args... args) {
			auto wrapped = get_call_context<W>();
			return wrapped->function(&wrapped->context, args...);
		}

		template <typename Wrapper, typename... Args>
		void emit_x86(SegmentedBuffer& buffer) {
			x86::BufferWriter writer (buffer);

#	if PLATFORM_WINDOWS
			x86::Registry arg_0 = x86::RCX;
#	elif PLATFORM_UNIX
			x86::Registry arg_0 = x86::RDI;
#	endif

			writer.section(MemoryFlag::X);
			writer.put_mov(x86::RAX, set_call_context<Wrapper>);

#if PLATFORM_UNIX
			writer.put_push(x86::RDI); // sysv
			writer.put_push(x86::RSI); // sysv
#endif

			writer.put_push(x86::RDX); // sysv, windows
			writer.put_push(x86::RCX); // sysv, windows
			writer.put_push(x86::R8);  // sysv, windows
			writer.put_push(x86::R9);  // sysv, windows

			writer.put_mov(arg_0, "context");
			writer.put_call(x86::RAX);
			writer.put_mov(x86::RAX, forward_with_context<Wrapper, Args...>);

			writer.put_pop(x86::R9);
			writer.put_pop(x86::R8);
			writer.put_pop(x86::RCX);
			writer.put_pop(x86::RDX);

#if PLATFORM_UNIX
			writer.put_pop(x86::RSI);
			writer.put_pop(x86::RDI);
#endif

			writer.put_jmp(x86::RAX); // tail call
		}

		template <typename Wrapper, typename... Args>
		void emit_riscv(SegmentedBuffer& buffer) {
			riscv::BufferWriter writer (buffer);

			constexpr int64_t size = 8 * 10;

			writer.section(MemoryFlag::X);
			writer.put_add(riscv::SP, riscv::SP, -size);
			writer.put_sq(riscv::RA, riscv::SP, 0*8); // return address
			writer.put_sq(riscv::A0, riscv::SP, 1*8);
			writer.put_sq(riscv::A1, riscv::SP, 2*8);
			writer.put_sq(riscv::A2, riscv::SP, 3*8);
			writer.put_sq(riscv::A3, riscv::SP, 4*8);
			writer.put_sq(riscv::A4, riscv::SP, 5*8);
			writer.put_sq(riscv::A5, riscv::SP, 6*8);
			writer.put_sq(riscv::A6, riscv::SP, 7*8);
			writer.put_sq(riscv::A7, riscv::SP, 8*8);

			writer.put_mov(riscv::T0, reinterpret_cast<uint64_t>(set_call_context<Wrapper>));
			writer.put_mov(riscv::A0, "context");
			writer.put_jalr(riscv::RA, riscv::T0);

			// restore
			writer.put_lq(riscv::RA, riscv::SP, 0*8);
			writer.put_lq(riscv::A0, riscv::SP, 1*8);
			writer.put_lq(riscv::A1, riscv::SP, 2*8);
			writer.put_lq(riscv::A2, riscv::SP, 3*8);
			writer.put_lq(riscv::A3, riscv::SP, 4*8);
			writer.put_lq(riscv::A4, riscv::SP, 5*8);
			writer.put_lq(riscv::A5, riscv::SP, 6*8);
			writer.put_lq(riscv::A6, riscv::SP, 7*8);
			writer.put_lq(riscv::A7, riscv::SP, 8*8);

			writer.put_add(riscv::SP, riscv::SP, size);

			writer.put_mov(riscv::T0, reinterpret_cast<uint64_t>(forward_with_context<Wrapper, Args...>));
			writer.put_jr(riscv::T0); // tail call
		}

		template <typename Wrapper, typename... Args>
		void emit_aarch64(SegmentedBuffer& buffer) {
			arm::BufferWriter writer (buffer);

			writer.section(MemoryFlag::X);
			writer.put_istp(arm::X29, arm::X30, arm::SP, -16);
			writer.put_istp(arm::X0, arm::X1, arm::SP, -16);
			writer.put_istp(arm::X2, arm::X3, arm::SP, -16);
			writer.put_istp(arm::X4, arm::X5, arm::SP, -16);
			writer.put_istp(arm::X6, arm::X7, arm::SP, -16);

			writer.put_mov(arm::X16, reinterpret_cast<uint64_t>(set_call_context<Wrapper>));
			writer.put_adr(arm::X0, "context");
			writer.put_blr(arm::X16);

			// restore
			writer.put_ldpi(arm::X6, arm::X7, arm::SP, 16);
			writer.put_ldpi(arm::X4, arm::X5, arm::SP, 16);
			writer.put_ldpi(arm::X2, arm::X3, arm::SP, 16);
			writer.put_ldpi(arm::X0, arm::X1, arm::SP, 16);
			writer.put_ldpi(arm::X29, arm::X30, arm::SP, 16);

			writer.put_mov(arm::X16, reinterpret_cast<uint64_t>(forward_with_context<Wrapper, Args...>));
			writer.put_br(arm::X16); // tail call
		}

		// Get a pointer to the size field of generated function,
		// that starts at the first byte of the second page
		inline uint64_t* get_function_size(uint8_t* function) {
			return reinterpret_cast<uint64_t*>(function + page_size());
		}

		// Generate the assembly trampoline and create the final raw function pointer
		template <typename R, util::nothrow_constuctable T, typename... Args>
		auto bind_native(R (* function) (const T* ctx, Args...), T context) -> R (*) (Args...) {

			using Wrapper = ContextWrapper<R, T, Args...>;

			SegmentedBuffer buffer;

			// We stores the context in thread_local, we could avoid that if we
			// handled the calling convention ourselves but that is hard and best left to the compiler.

#if ARCH_X86
			emit_x86<Wrapper, Args...>(buffer);
#elif ARCH_RISCV64
			emit_riscv<Wrapper, Args...>(buffer);
#elif ARCH_AARCH64
			emit_aarch64<Wrapper, Args...>(buffer);
#elif
#	error "Unimplemented architecture!"
#endif

			BasicBufferWriter writer(buffer);

			writer.section(MemoryFlag::R | MemoryFlag::W);
			writer.label("size");
			writer.put_qword(0); // buffer size will be written here
			writer.label("context");
			writer.put_space(sizeof(Wrapper));
			writer.put_space(util::align_padding(sizeof(Wrapper), static_cast<size_t>(16)));

			ExecutableBuffer exe = to_executable(buffer);
			const auto page = page_size();

			Wrapper* target = reinterpret_cast<Wrapper*>(exe.address("context"));
			std::construct_at(target, function, context);

			if (exe.size() < 2 * page) {
				throw std::runtime_error("Bound function too small, got " + std::to_string(exe.size()) + " bytes, expected " + std::to_string(2 * page) + " bytes!");
			}

			if (exe.offset("size") != page) {
				throw std::runtime_error("Size marker misplaced, got offset +" + std::to_string(exe.offset("size")) + ", expected +" + std::to_string(page) + "!");
			}

			*get_function_size(exe.address()) = exe.size();
			return reinterpret_cast<R (*) (Args...)>(exe.own());
		}

		template <typename>
		struct capture_forwarder {};

		template <typename... Args>
		struct capture_forwarder<std::tuple<Args...>> {

			// The function that gets bound to context in bind_native() for all lambdas
			// it then forward the call to the object held in the context - the actual lambda.
			template <typename L>
			static auto forward_lambda_call(const L* ctx_lambda, Args... args) noexcept(util::function_traits<L>::nothrow) {
				return (*ctx_lambda)(std::forward<Args>(args)...);
			}

			// Thin wrapper, we need this to unpack the parameter pack from the tuple object
			// we obtained from function_traits<T>::arguments.
			template <typename L>
			static auto coerce(L&& ctx_lambda) {
				return bind_native(forward_lambda_call<L>, std::forward<L>(ctx_lambda));
			}

		};

	}

	/*
	 * Free a raw allocated function object. Needs te be be used on pointers
	 * returned by function in this module after they are no longer in use to
	 * avoid a memory leak.
	 */
	template <typename R, typename... Args>
	void free_function(R (* function) (Args...)) {
		auto* buffer = reinterpret_cast<uint8_t*>(function);
		free_pages(buffer, *detail::get_function_size(buffer));
	}

	/**
	 * Bind a set of arguments to a function pointer and return a raw function pointer.
	 * This can be useful when interfacing with C code that lacks "userdata" call context.
	 * The returned raw pointer must be freed after use using free_function().
	 */
	template <typename R, typename... Args>
	auto specialize(R (* function) (Args... args), Args... args) -> R (*) () {
		struct Context {
			R (* function) (Args... args);
			std::tuple<Args...> args;
		};

		Context ctx {};
		ctx.function = function;
		ctx.args = {args...};

		using Trampoline = R (*) (const Context* ctx);

		Trampoline func = [] (const Context* ctx) {
			return std::apply(ctx->function, ctx->args);
		};

		return detail::bind_native(func, std::move(ctx));
	}

	/**
	 * Convert a capturing (non-polymorphic) lambda into a raw function pointer.
	 * This can be useful when interfacing with C code that lacks "userdata" call context.
	 * The returned raw pointer must be freed after use using free_function().
	 */
	template <typename F> requires util::functional<F>
	auto decay(F callable) -> util::function_traits<F>::pointer_type {
		using trait = util::function_traits<F>;

		if constexpr (util::decayable_lambda<F>) {
			return + callable;
		} else {
			return detail::capture_forwarder<typename trait::arguments>::coerce(std::move(static_cast<trait::capture_type>(callable)));
		}
	}

}