#pragma once

#include <asmio/external.hpp>

#define ENUM_BEGIN __INTERNAL__ = __LINE__,
#define ENUM_END ENUM_LENGTH = __LINE__ - __INTERNAL__ - 1,

namespace asmio::util {

	template <typename T>
	concept integer_like = std::is_integral_v<T> || std::is_enum_v<T>;

	template <typename T>
	concept nothrow_constuctable = std::is_nothrow_constructible_v<T>;

	template <typename T>
	concept trivially_copyable = std::is_trivially_copyable_v<T>;

	template <typename T, typename A>
	concept castable = requires (const A& arg) { static_cast<T>(arg); };

	template <typename T>
	concept is_enumeration = requires { std::is_enum_v<T>; };

	template <is_enumeration T>
	constexpr size_t enum_length = static_cast<size_t>(T::ENUM_LENGTH);

	template <typename R, typename... Args>
	struct base_function_traits {
		using return_type = R;
		using arguments = std::tuple<Args...>;

		template <size_t index>
		using arg_type = std::tuple_element_t<index, arguments>;

		static constexpr size_t arity = sizeof...(Args);

		using function_type = R(Args...);
		using capture_type = std::function<function_type>;
		using pointer_type = std::add_pointer_t<function_type>;
	};

	template <typename>
	struct function_traits;

	// function type
	template <typename R, typename... Args>
	struct function_traits<R(Args...)> : base_function_traits<R, Args...> {
		static constexpr bool nothrow = false;
	};

	template <typename R, typename... Args>
	struct function_traits<R(Args...) noexcept> : base_function_traits<R, Args...> {
		static constexpr bool nothrow = true;
	};

	// function pointer
	template <typename R, typename... Args>
	struct function_traits<R(*)(Args...)> : function_traits<R(Args...)> {};

	template <typename R, typename... Args>
	struct function_traits<R(*)(Args...) noexcept> : function_traits<R(Args...) noexcept> {};

	// member function pointer
	template <typename R, typename C, typename... Args>
	struct function_traits<R(C::*)(Args...)> : function_traits<R(Args...)> {};

	template <typename R, typename C, typename... Args>
	struct function_traits<R(C::*)(Args...) noexcept> : function_traits<R(Args...) noexcept> {};

	// const member function pointer
	template <typename R, typename C, typename... Args>
	struct function_traits<R(C::*)(Args...) const> : function_traits<R(Args...)> {};

	template <typename R, typename C, typename... Args>
	struct function_traits<R(C::*)(Args...) const noexcept> : function_traits<R(Args...) noexcept> {};

	// functors / lambdas
	template <typename F>
	struct function_traits : function_traits<decltype(&F::operator())> {};

	template <typename T>
	concept functional = requires { typename function_traits<T>; };

	template <typename T>
	concept decayable_lambda = functional<T> && requires(T lambda) { { + lambda } -> std::same_as<typename function_traits<T>::pointer_type>; };

}