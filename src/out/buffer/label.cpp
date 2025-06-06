
#include "label.hpp"

namespace asmio {

	Label::Label(const std::string& str)
	: str(static_cast<const char*>(malloc(str.length()))), hash(util::hash_djb2(str.c_str())), allocated(false) {
		memcpy((void*) this->str, str.c_str(), str.size() + 1);
	}

	Label::Label(const Label& label)
	: str(label.str), hash(label.hash), allocated(label.allocated) {
		if (allocated) {
			size_t len = strlen(str) + 1;
			str = static_cast<const char*>(malloc(len));
			memcpy((void*) str, label.str, len);
		}
	}

	Label::~Label() {
		if (allocated) free((void*) str);
	}

}