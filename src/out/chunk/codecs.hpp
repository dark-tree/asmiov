#pragma once

#include "buffer.hpp"

namespace asmio {

	template <typename C, typename T>
	struct Encodec {
		using codec = C;
		const T value;

		static void encode(ChunkBuffer& buffer, Encodec encodable) {
			buffer.put<C>(encodable.value);
		}
	};

	template <typename C, typename T>
	static Encodec<C, T> make_encodec(const T& value) {
		return {value};
	}

	template <typename T>
	struct is_encodec : std::false_type {};

	template <typename A, typename B>
	struct is_encodec<Encodec<A, B>> : std::true_type {};

	template <typename T>
	concept any_encodec = is_encodec<T>::value;

	/*
	 * https://en.wikipedia.org/wiki/LEB128
	 */

	struct UnsignedLeb128 {
		static void encode(ChunkBuffer& buffer, uint64_t value);
	};

	struct SignedLeb128 {
		static void encode(ChunkBuffer& buffer, int64_t signed_value);
	};

	struct DynamicInt {
		static void encode(ChunkBuffer& buffer, std::tuple<int, uint64_t> value);
	};

}