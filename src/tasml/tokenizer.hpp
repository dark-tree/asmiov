#pragma once

#include "external.hpp"
#include "token.hpp"

namespace asmio {

	std::vector<Token> tokenize(const std::string &input);

}