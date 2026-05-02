#pragma once

#include <asmio/util/refcnt.hpp>
#include <asmio/external.hpp>
#include <asmio/util.hpp>

namespace asmio {

	/// Universal buffer label
	struct Label {

		private:

			// a Label can exist in one of 4 distinct states, the empty state represents a Label
			// that will never compare as equal to any other Label, except for the empty Label itself.
			//
			// Condition                                  | Union      | Description                            //
			// ------------------------------------------ + ---------- + -------------------------------------- //
			// length != 0, hash != 0, allocated == true  | Label::ptr | owns a null-byte terminated c-string   //
			// length != 0, hash != 0, allocated == false | Label::str | points into an external char span      //
			// length == 0, hash != 0, allocated == false | Label::id  | 64-bit integer based unique identifier //
			// length == 0, hash == 0, allocated == false |            | empty, does not contain any reference  //

			union {
				char* ptr;       // used for allocated labels, points at the string right after the RefHeader
				const char* str; // used for const strings
				uint64_t id;     // used for ID only Labels
			};

			uint8_t allocated : 1;
			uint32_t length : 31;
			uint32_t hash;

			constexpr explicit Label(uint64_t id)
				: id(id), allocated(false), length(0), hash(util::hash_tmix64(id)) {
			}

		public:

			static Label UNSET;

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
				return hash == 0;
			}

			constexpr Label()
				: id(0), allocated(false), length(0), hash(0) {
			}

			constexpr Label(std::nullptr_t ignored)
				: Label() {
				(void) ignored;
			}

			constexpr Label(Label&& label) noexcept
				: id(label.id), allocated(label.allocated), length(label.length), hash(label.hash) {
				label.allocated = false;
			}

			constexpr Label(const Label& label) noexcept
				: id(label.id), allocated(label.allocated), length(label.length), hash(label.hash) {

				// increase the ref-count
				if (label.allocated) {
					ref_increment(ptr);
				}
			}

			Label(const char* str);
			Label(const std::string_view& view);
			Label(const std::string& str);
			~Label();

			/// Create a non-owning label of the given string
			static Label of(const char* str);

			/// Create a non-owning label of the given string
			static Label of(const std::string_view& view);

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
				if (empty()) return "$unset";
				if (!is_text()) return "$anonymous";

				return std::string_view {str, str + length};
			}

			/// Get string label as std::string
			std::string string() const {
				if (empty()) return "$unset";
				if (!is_text()) return "$anonymous:" + std::to_string(id);

				return std::string {view()};
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