#pragma once

#include <asmio/external.hpp>

namespace asmio::riscv {

	enum struct Condition : uint8_t {
		EQ  = 0x00, NE  = 0x01,
		LT  = 0x04, GT  = 0x14,
		GE  = 0x05, LE  = 0x15,
		LTU = 0x06, GTU = 0x16,
		GEU = 0x07, LEU = 0x17,
	};

	/**
	 * Extract the condition code used during instruction encoding
	 */
	constexpr uint8_t get_condition_code(Condition cond) {
		return static_cast<uint8_t>(cond) & 0xf;
	}

	/**
	 * Check if during instruction encoding this condition requires inverting the arguments
	 */
	constexpr bool get_condition_swap(Condition cond) {
		return static_cast<uint8_t>(cond) & 0x10;
	}

	inline Condition parse_condition_enum(const std::string_view& view) {
		if (view == "eq") return Condition::EQ;
		if (view == "ne") return Condition::NE;
		if (view == "lt") return Condition::LT;
		if (view == "ge") return Condition::GE;
		if (view == "gt") return Condition::GT;
		if (view == "le") return Condition::LE;
		if (view == "ltu") return Condition::LTU;
		if (view == "geu") return Condition::GEU;
		if (view == "gtu") return Condition::GTU;
		if (view == "leu") return Condition::LEU;

		throw std::runtime_error {"Invalid condition enumeration"};
	}

}