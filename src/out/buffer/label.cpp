
#include "label.hpp"

namespace asmio {

	/*
	 * class Label
	 */

	Label Label::UNSET {};

	Label::Label(const char* str) {

		if (str == nullptr) {
			allocated = false;
			length = 0;
			hash = 0;
			id = 0;
			return;
		}

		length = strlen(str);

		if (length == 0) {
			throw std::runtime_error {"Label text can't be empty!"};
		}

		allocated = true;
		ptr = ref_allocate<char>(length);
		hash = util::hash_djb2(str, length);

		memcpy(ptr, str, length);
	}

	Label::Label(const std::string_view& view) {
		length = view.length();

		if (length == 0) {
			throw std::runtime_error {"Label text can't be empty!"};
		}

		allocated = true;
		ptr = ref_allocate<char>(length);
		hash = util::hash_djb2(view.data(), length);

		memcpy(ptr, view.data(), length);
	}

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

		ptr = 0;
		hash = 0;
		length = 0;
	}

	Label Label::of(const char* str) {
		Label label {};

		label.str = str;
		label.allocated = false;

		if (str == nullptr) {
			label.length = 0;
			label.hash = 0;
			return label;
		}

		label.length = strlen(str);
		label.hash = util::hash_djb2(str, label.length);

		if (label.length == 0) {
			throw std::runtime_error {"Label text can't be empty!"};
		}

		return label;
	}

	Label Label::of(const std::string_view& view) {
		Label label {};

		label.allocated = false;
		label.str = view.data();
		label.length = view.length();
		label.hash = util::hash_djb2(label.str, label.length);

		if (label.length == 0) {
			throw std::runtime_error {"Label text can't be empty!"};
		}

		return label;
	}

}