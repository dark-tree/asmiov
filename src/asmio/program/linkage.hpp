#pragma once

#include "segmented.hpp"

namespace asmio {

	struct LinkageType {

		static Linkage::Type RISCV_BRANCH;
		static Linkage::Type RISCV_JUMP;

		static Linkage::Type AARCH64_21_5_LO_HI;
		static Linkage::Type AARCH64_14_5_ALIGNED;
		static Linkage::Type AARCH64_19_5_ALIGNED;
		static Linkage::Type AARCH64_26_0_ALIGNED;

		static Linkage::Type X86_64_ABSOLUTE;
		static Linkage::Type X86_64_RELATIVE;
		static Linkage::Type X86_32_ABSOLUTE;
		static Linkage::Type X86_32_SIGN_ABSOLUTE;
		static Linkage::Type X86_32_SIGN_RELATIVE;
		static Linkage::Type X86_16_ABSOLUTE;
		static Linkage::Type X86_16_SIGN_RELATIVE;
		static Linkage::Type X86_8_SIGN_ABSOLUTE;
		static Linkage::Type X86_8_SIGN_RELATIVE;

	};

}