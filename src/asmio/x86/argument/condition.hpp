#pragma once

#include <asmio/external.hpp>

namespace asmio::x86 {

	enum struct SimdCondition : uint8_t {
		EQ  = 0, ///< Equal              (ordered,   non-signaling)
		LT  = 1, ///< Less-than          (ordered,       signaling)
		LE  = 2, ///< Less-than-or-equal (ordered,       signaling)
		RD  = 7, ///< Ordered            (ordered,   non-signaling)
		NEQ = 4, ///< Not-equal          (unordered, non-signaling)
		NLT = 5, ///< Not-less-than      (unordered,     signaling)
		NLE = 6, ///< Not-greater-than   (unordered,     signaling)
		NRD = 3, ///< Unordered          (unordered, non-signaling)
	};

	enum struct Condition : uint8_t {
		O   = 0x0,                       ///< Overflow
		NO  = 0x1,                       ///< Not Overflow
		B   = 0x2,   C = 0x2, NAE = 0x2, ///< Below, Carry, Not Above or Equal
		NB  = 0x3,  NC = 0x3,  AE = 0x3, ///< Not Below, Not Carry, Above or Equal
		E   = 0x4,   Z = 0x4,            ///< Equal, Zero
		NE  = 0x5,  NZ = 0x5,            ///< Not Equal, Not Zero
		BE  = 0x6,  NA = 0x6,            ///< Below or Equal, Not Above
		NBE = 0x7,   A = 0x7,            ///< Not Below or Equal, Above
		S   = 0x8,                       ///< Sign
		NS  = 0x9,                       ///< Not Sign
		P   = 0xA,  PE = 0xA,            ///< Parity, Parity Even
		NP  = 0xB,  PO = 0xB,            ///< Not Parity, Parity Odd
		L   = 0xC, NGE = 0xC,            ///< Less, Not Greater or Equal
		NL  = 0xD,  GE = 0xD,            ///< Not Less, Greater or Equal
		LE  = 0xE,  NG = 0xE,            ///< Less or Equal, Not Greater
		NLE = 0xf,   G = 0xF,            ///< Not Less or Equal, Greater
	};

}