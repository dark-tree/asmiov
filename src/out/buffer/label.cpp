
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
		ptr = reinterpret_cast<char*>(malloc(sizeof(ref_header) + str.size())) + sizeof(ref_header);
		*get_refcount() = 1;

		length = str.length();
		hash = util::hash_djb2(str.c_str(), length);
		memcpy(ptr, str.c_str(), length);
	}

	Label::~Label() {
		if (allocated) {
			allocated = false;

			ref_header* header = get_refcount();
			*header -= 1;

			if (*header == 0) {
				free(header);
			}
		}
	}

}