#pragma once

#include <asmio/external.hpp>

namespace asmio::arm {

	// Specifies ordering semantics of instructions,
	// learn more about ordering semantics: https://davekilian.com/acquire-release.html
	enum struct Order : uint8_t {
		//                  L        o0
		NONE            = 0b00'00000'0, ///< Ensure no ordering of operations
		RELEASE         = 0b00'00000'1, ///< Ensure that the preceding operations finish before this one starts
		ACQUIRE         = 0b10'00000'0, ///< Ensure that the following operations wait for this one to finish
		ACQUIRE_RELEASE = 0b10'00000'1, ///< Ensure that both previous and preceding operations are executed in order

		// "L" flag is used to mark acquire semantics (marked with suffix "A" in mnemonics),
		// and o0 to mark "release" (suffix "L" in mnemonics). How designed it like this??
	};

	/**
	 * Check if Order is ACQUIRE or ACQUIRE_RELEASE
	 */
	constexpr bool is_order_acquire(Order order) {
		return static_cast<uint8_t>(order) & static_cast<uint8_t>(Order::ACQUIRE);
	}

	/**
	 * Check if Order is RELEASE or ACQUIRE_RELEASE
	 */
	constexpr bool is_order_release(Order order) {
		return static_cast<uint8_t>(order) & static_cast<uint8_t>(Order::RELEASE);
	}

	/**
	 * Convert Order to a bit mask used during instruction encoding
	 */
	constexpr uint32_t order_to_dword_mask(Order order) {
		return static_cast<uint32_t>(order) << 15;
	}

}