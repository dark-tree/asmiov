#pragma once

#include "external.hpp"
#include "util.hpp"

namespace asmio {

	/// Universal buffer label
	struct Label {

		private:

			const char* str;
			uint64_t hash;
			bool allocated;

		public:

			constexpr Label(const char* str)
			: str(str), hash(util::hash_djb2(str)), allocated(false) {}

			constexpr Label(const Label&& label)// move
			: str(label.str), hash(label.hash), allocated(label.allocated) {}

			Label(const std::string& str);
			Label(const Label& label); // copy

			~Label();

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

	template<typename T>
	using LabelMap = std::unordered_map<Label, T, Label::HashFunction>;

}