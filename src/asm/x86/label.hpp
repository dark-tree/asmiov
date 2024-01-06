#pragma once

#include "external.hpp"
#include "util.hpp"
#include "const.hpp"

namespace asmio::x86 {

	struct Label {

		private:

			const char* str;
			uint64_t hash;
			bool allocated;

		public:

			Label(const char* str)
			: str(str), hash(hash_djb2(str)), allocated(false) {}

			Label(const std::string& str)
			: str((const char*) malloc(str.length())), hash(hash_djb2(str.c_str())), allocated(false) {
				memcpy((void*) this->str, str.c_str(), str.size() + 1);
			}

			Label(const Label& label)
			: str(label.str), hash(label.hash), allocated(label.allocated) {
				if (allocated) {
					size_t len = strlen(str) + 1;
					str = (const char*) malloc(len);
					memcpy((void*) str, label.str, len);
				}
			}

			Label(const Label&& label)
			: str(label.str), hash(label.hash), allocated(label.allocated) {}

			~Label() {
				if (allocated) free((void *) str);
			}

			bool operator == (const Label& label) const {
				return label.hash == this->hash && strcmp(label.str, this->str) == 0;
			}

			const char* c_str() const {
				return str;
			}

			struct HashFunction {

				size_t operator () (const Label &label) const {
					return label.hash;
				}

			};

	};

	struct LabelCommand {

		Label label;
		uint8_t size;
		long offset;
		long shift;
		bool relative;

	};

}