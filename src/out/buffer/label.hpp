#pragma once

#include "external.hpp"
#include "../../asm/util.hpp"

namespace asmio {

	/// Universal buffer label
	struct Label {

		private:

			union {
				const char* str; // used for const strings
				uint64_t id;     // used for ID only Labels
				void* ptr;       // used for allocated labels
			};

			uint8_t allocated : 1;
			uint32_t length : 31;
			uint32_t hash;

			constexpr explicit Label(uint64_t id)
				: id(id), allocated(false), length(0), hash(util::hash_tmix64(id)) {
			}

		public:

			static Label make_unique() {
				static uint64_t counter = 1;
				Label label {counter ++};
				return label;
			}

			constexpr uint64_t hashed() const {
				const uint64_t value = length;
				return value << 32 | hash;
			}

			constexpr bool is_text() const {
				return length != 0;
			}

			constexpr bool empty() const {
				return is_text() && (id == 0);
			}

			constexpr Label()
				: id(0), allocated(false), length(0xFFFF), hash(0) {
			}

			constexpr Label(nullptr_t)
				: Label() {
			}

			constexpr Label(const char* str)
			: str(str), allocated(false) {
				if (str == nullptr) {
					length = 0xFFFE;
					hash = 0;
					return;
				}

				length = strlen(str);
				hash = util::hash_djb2(str, length);

				if (length == 0) {
					throw std::runtime_error {"Label text can't be empty!"};
				}
			}

			constexpr Label(const std::string_view& view)
			: allocated(false) {
				str = view.data();
				length = view.length();
				hash = util::hash_djb2(str, length);

				if (length == 0) {
					throw std::runtime_error {"Label text can't be empty!"};
				}
			}

			constexpr Label(Label&& label) noexcept
				: id(label.id), allocated(label.allocated), length(label.length), hash(label.hash) {
				label.allocated = false;
			}

			constexpr Label(const Label& label) noexcept
				: id(label.id), allocated(label.allocated), length(label.length), hash(label.hash) {

				if (label.allocated) {
					ptr = malloc(length);
					memcpy(ptr, label.ptr, length);
				}
			}

			Label(const std::string& str);
			~Label();

			/// Compare two labels
			constexpr bool operator == (const Label& label) const {
				if (label.hashed() != this->hashed()) {
					return false;
				}

				// we know the lengths are the same, as the hash_view includes it
				// this loop will be completely skipped for non-text labels as then length == 0
				for (uint64_t i = 0; i < length; i ++) {
					if (label.str[i] != str[i]) return false;
				}

				if (!is_text()) {
					return id == label.id;
				}

				return true;
			}

			/// Get string label as std::string_view
			constexpr std::string_view view() const {
				return length == 0 ? "$internal" : std::string_view {str, str + length};
			}

			/// Get string label as std::string
			std::string string() const {
				return length == 0 ? "$internal:" + std::to_string(id) : std::string {view()};
			}

			/// Function used in hashmaps to get the elements hash value
			struct HashFunction {
				// we don't use .hashed() here as it has worse avalanche effect than just pure .hash
				constexpr size_t operator () (const Label& label) const noexcept { return label.hash; }
			};

	};

	template<typename T>
	using LabelMap = std::unordered_map<Label, T, Label::HashFunction>;

}