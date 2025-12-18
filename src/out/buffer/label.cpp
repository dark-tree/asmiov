
#include "label.hpp"

namespace asmio {

	/*
	 * class Label
	 */

	Label Label::UNSET {};

	Label::Label(const std::string& str) {
		if (str.empty()) {
			throw std::runtime_error {"Label text can't be empty!"};
		}

		allocated = true;
		ptr = ref_allocate<char>(str.size());

		length = str.length();
		hash = util::hash_djb2(str.c_str(), length);
		memcpy(ptr, str.c_str(), length);
	}

	Label::~Label() {
		if (allocated) {
			allocated = false;
			ref_free(ptr);
		}
	}

}