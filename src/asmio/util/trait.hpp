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

	template <typename, typename...>
	struct function_decompose : std::false_type {};

	template <typename R, typename... A>
	struct function_decompose<R(A...)> {
		using return_type = R;
		using arguments = std::tuple<A...>;

		template <size_t index>
		using arg_type = std::tuple_element_t<index, arguments>;
	};

}