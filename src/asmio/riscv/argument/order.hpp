#pragma once

#include <asmio/external.hpp>

namespace asmio::riscv {

	// Copied from the AArch64
	// TODO: unify?
	enum struct Order : uint8_t {
		NONE            = 0b00, ///< Ensure no ordering of operations
		RELEASE         = 0b01, ///< Ensure that the preceding operations finish before this one starts
		ACQUIRE         = 0b10, ///< Ensure that the following operations wait for this one to finish
		ACQUIRE_RELEASE = 0b11, ///< Ensure that both previous and preceding operations are executed in order
	};

	constexpr Order parse_order_enum(const std::string_view& view) {
		if (view == "none") return Order::NONE;
		if (view == "rl") return Order::RELEASE;
		if (view == "aq") return Order::ACQUIRE;
		if (view == "ra" || view == "ar" || view == "aqrl" || view == "rlaq") return Order::ACQUIRE_RELEASE;

		throw std::runtime_error {"Invalid order enumeration"};
	}

}