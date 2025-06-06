#pragma once

#include "external.hpp"
#include "util.hpp"

namespace asmio {

	struct Label {

		private:

			const char* str;
			uint64_t hash;
			bool allocated;

		public:

			Label(const char* str)
			: str(str), hash(util::hash_djb2(str)), allocated(false) {}

			Label(const std::string& str)
			: str(static_cast<const char*>(malloc(str.length()))), hash(util::hash_djb2(str.c_str())), allocated(false) {
				memcpy((void*) this->str, str.c_str(), str.size() + 1);
			}

			Label(const Label& label)
			: str(label.str), hash(label.hash), allocated(label.allocated) {
				if (allocated) {
					size_t len = strlen(str) + 1;
					str = static_cast<const char*>(malloc(len));
					memcpy((void*) str, label.str, len);
				}
			}

			constexpr Label(const Label&& label)
			: str(label.str), hash(label.hash), allocated(label.allocated) {}

			~Label() {
				if (allocated) free((void*) str);
			}

			/// Compare two labels
			constexpr bool operator == (const Label& label) const {
				return label.hash == this->hash && strcmp(label.str, this->str) == 0;
			}

			/// Get label string
			constexpr const char* c_str() const {
				return str;
			}

			/// Function used in hashmaps to get the elements hash value
			struct HashFunction {
				constexpr size_t operator () (const Label& label) const { return label.hash; }
			};

	};

}