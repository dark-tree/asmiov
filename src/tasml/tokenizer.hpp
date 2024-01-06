#pragma once

#include "external.hpp"
#include "token.hpp"

namespace asmio::tasml {

	std::vector<Token> tokenize(const std::string &input);

}