#pragma once

#include <asmio/util/bits.hpp>
#include <asmio/util/hash.hpp>
#include <asmio/util/string.hpp>
#include <asmio/util/trait.hpp>

#define EXIT_OK 0
#define EXIT_ERROR 1

#define ASMIOV_VERSION "1.0.0"
#define ASMIOV_SOURCE "https://github.com/dark-tree/asmiov"

namespace asmio::util {

	template <typename F, typename T>
	struct UniqueHandle {

		private:

			friend F; // wait, you can do that?
			T m_handle;

			constexpr UniqueHandle(T handle)
				: m_handle(handle) {
			}

		public:

			const T& handle() const {
				return m_handle;
			}

	};

	/// Unsigned divide (round up)
	template <std::integral T>
	constexpr auto divide_up(T a, T b) {
		return (a + b - 1) / b;
	}

	/// Align 'value' to a multiple of 'alignment'
	template <std::integral T>
	constexpr auto align_up(T value, T alignment) {
		return divide_up(value, alignment) * alignment;
	}

	/// Compute the number that needs to be added to 'value' so that it is a multiple of 'alignment'
	template <std::integral T>
	constexpr auto align_padding(T value, T alignment) {
		return align_up(value, alignment) - value;
	}

	/// Convert value to the given endian from the native system alignment
	template <std::integral T>
	constexpr auto native_to_endian(T value, std::endian endian) {
		return std::endian::native == endian ? value : std::byteswap(value);
	}

	template<typename T>
	auto get_int_or(T value) {
		if constexpr (std::is_pointer_v<T>) return 0; else return value;
	}

	template<typename T>
	auto get_ptr_or(T value) {
		if constexpr (std::is_pointer_v<T>) return value; else return nullptr;
	}

	/// Iterate the container and check if it contains the given value
	template<typename T>
	bool contains(const T& container, const auto& value) {
		for (const auto& element : container) {
			if (element == value) return true;
		}

		return false;
	}

}