#include <asmio/riscv/module.hpp>

#include "test.hpp"
#include "vstl.hpp"

namespace test {

	using namespace asmio;
	using namespace asmio::riscv;

	TEST (riscv_module) {

		LanguageModule module;
		CHECK(std::string(module.name()), "riscv");

	};

}