#pragma once

#include <asmio/external.hpp>

namespace asmio::riscv {

	enum struct Condition : uint8_t {
		EQ = 0x0,
		NE = 0x1,
		LT = 0x4,
		GE = 0x5,
		LTU = 0x6,
		GEU = 0x7
	};

	inline Condition parse_condition_enum(const std::string_view& view) {
		if (view == "eq") return Condition::EQ;
		if (view == "ne") return Condition::NE;
		if (view == "lt") return Condition::LT;
		if (view == "ge") return Condition::GE;
		if (view == "ltu") return Condition::LTU;
		if (view == "geu") return Condition::GEU;

		throw std::runtime_error {"Invalid condition enumeration"};
	}

}