#pragma once

#include <asmio/external.hpp>

namespace asmio::util {

	/**
	 * Get the minimal number of bytes (in power-of-two increments) needed to encode an unsigned value
	 */
	constexpr int min_unsigned_bytes(uint64_t value) {
		if (value > 0xFFFFFFFF) return 8;
		if (value > 0xFFFF) return 4;
		if (value > 0xFF) return 2;

		return 1;
	}

	/**
	 * Check how many bits can be truncated from a signed number before
	 * it changed its value, assuming one bit is needed for the sign.
	 */
	constexpr int count_redundant_sign_bits(const int64_t value) {
		return __builtin_clzll(value >= 0 ? value : ~value) - 1;
	}

	/**
	 * Check if given number signed number can be losslessly
	 * encoded in the given number of bits, taking into account the sign bits.
	 */
	constexpr bool is_signed_encodable(int64_t value, int64_t bits) {
		return (64 - count_redundant_sign_bits(value)) <= bits;
	}

	/**
	 * Count the number of 'zeros' form the trailing (least
	 * significant) side of a number.
	 */
	constexpr int count_trailing_zeros(uint64_t value) {
		return __builtin_ctzll(value);
	}

	/**
	 * Count the number of 'ones' form the trailing (least
	 * significant) side of a number.
	 */
	constexpr int count_trailing_ones(uint64_t value) {
		return count_trailing_zeros(~value); // ctz(~x) == cto(x)
	}

	/**
	 * Count the number of 'zeros' form the leading (most
	 * significant) side of a number.
	 */
	constexpr int count_leading_zeros(uint64_t value) {
		return __builtin_clzll(value);
	}

	/**
	 * Count the number of 'ones' form the leading (most
	 * significant) side of a number.
	 */
	constexpr int count_leading_ones(uint64_t value) {
		return count_leading_zeros(~value); // clz(~x) == clo(x)
	}

	/**
	 * Count the number of repeating bits form the leading (most
	 * significant) side of a number.
	 */
	constexpr int count_leading(uint64_t value) {
		return std::max(count_leading_zeros(value), count_leading_ones(value));
	}

	/**
	 * Create a constant with a specific number of bits set,
	 * starting on the least-significant side.
	 */
	template <std::integral T>
	constexpr T bit_fill(uint64_t count) {
		if (count >= sizeof(T) * 8) {
			return std::numeric_limits<T>::max();
		}

		return (T(1) << count) - T(1);
	}

	/**
	 * Get the minimal number of bytes (in power-of-two increments) needed
	 * to losslessly encode the given signed integer.
	 */
	constexpr int min_signed_bytes(int64_t value) {
		const uint64_t uval = static_cast<uint64_t>(value);

		if ((value & 0xFFFF'FFFF'FFFF'FF80) == 0xFFFF'FFFF'FFFF'FF80) return 1; // 1 byte long negative
		if (uval <= 0x0000'0000'0000'007F) return 1; // 1 byte long positive

		if ((value & 0xFFFF'FFFF'FFFF'8000) == 0xFFFF'FFFF'FFFF'8000) return 2; // 2 byte long negative
		if (uval <= 0x0000'0000'0000'7FFF) return 2; // 2 byte long positive

		if ((value & 0xFFFF'FFFF'8000'0000) == 0xFFFF'FFFF'8000'0000) return 4; // 4 byte long negative
		if (uval <= 0x0000'0000'7FFF'FFFF) return 4; // 4 byte long positive

		return 8;
	}

	constexpr int min_optimistic_bytes(uint64_t value) {
		return std::min(min_unsigned_bytes(value), min_signed_bytes(value));
	}


}