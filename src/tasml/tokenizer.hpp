#pragma once

#include "external.hpp"
#include "token.hpp"
#include "error.hpp"

namespace tasml {

	std::vector<Token> tokenize(ErrorHandler& reporter, const std::string &input);

}