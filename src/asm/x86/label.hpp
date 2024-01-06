#pragma once

#include "external.hpp"
#include "util.hpp"
#include "const.hpp"

namespace asmio::x86 {

	struct Label {

		private:

			const char* str;
			uint64_t hash;

		public:

			Label(const char* str)
			: str(str), hash(hash_djb2(str)) {}

			Label(const std::string& str)
			: Label(str.c_str()) {}

			bool operator == (Label label) const {
				return label.hash == this->hash && strcmp(label.str, this->str) == 0;
			}

			const char* c_str() {
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