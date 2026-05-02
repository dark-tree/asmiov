#pragma once

#include <asmio/external.hpp>

namespace asmio::arm {

	// Specifies ordering semantics of instructions,
	// learn more about ordering semantics: https://davekilian.com/acquire-release.html
	enum struct Order : uint8_t {
		NONE            = 0b00, ///< Ensure no ordering of operations
		RELEASE         = 0b01, ///< Ensure that the preceding operations finish before this one starts
		ACQUIRE         = 0b10, ///< Ensure that the following operations wait for this one to finish
		ACQUIRE_RELEASE = 0b11, ///< Ensure that both previous and preceding operations are executed in order
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

}