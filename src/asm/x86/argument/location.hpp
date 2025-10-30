#pragma once

#include "external.hpp"
#include "out/buffer/sizes.hpp"
#include "registry.hpp"
#include "scaled.hpp"
#include "out/buffer/label.hpp"
#include "../../util.hpp"

namespace asmio::x86 {

	/// Used to represent any valid x86 instruction argument
	class PACKED Location {

		public:

			const Registry base;
			const Registry index;
			const uint8_t scale  : 4; // the scale, as integer
			const bool reference : 4; // is this location a memory reference
			const uint8_t size;
			const int64_t offset;
			const char* label;

			constexpr void check_non_referential(const char* why) const {
				if (reference) throw std::runtime_error {why};
			}

		public:

			template<typename T>
			constexpr Location(T offset = 0) // can the same thing be archived with some smart overload?
			: Location(UNSET, UNSET, 1, util::get_int_or(offset), util::get_ptr_or(offset), VOID, false) {}

			constexpr Location(Registry registry)
			: Location(registry, UNSET, 1, 0, nullptr, registry.size, false) {}

			constexpr Location(ScaledRegistry index)
			: Location(UNSET, index.registry, index.scale, 0, nullptr, index.registry.size, false) {}

			constexpr explicit Location(Registry base, Registry index, uint32_t scale, int64_t offset, const char* label, int size, bool reference)
			: base(base), index(index), scale(scale), reference(reference), size(size), offset(offset), label(label) {
				check_valid_scale(index, scale);
			}

		public:

			constexpr Location ref() const {
				check_non_referential("Can't reference a reference!");
				return Location {base, index, scale, offset, label, VOID, true};
			}

			constexpr Location cast(uint8_t bytes) const {
				if (reference || is_immediate()) {
					return Location {base, index, scale, offset, label, bytes, reference};
				}

				throw std::runtime_error {"The result of this expression is of fixed size!"};
			}

			constexpr Location operator + (int64_t extend) const {
				check_non_referential("Can't modify a reference!");
				return Location {base, index, scale, offset + extend, label, size, false};
			}

			constexpr Location operator - (int64_t extend) const {
				check_non_referential("Can't modify a reference!");
				return Location {base, index, scale, offset - extend, label, size, false};
			}

			constexpr Location operator + (const char* label) const {
				check_non_referential("Can't modify a reference!");
				return Location {base, index, scale, offset, label, size, false};
			}

			/// Check if the size of this element hasn't yet been defined
			constexpr bool is_indeterminate() const {
				return size == VOID;
			}

			/// Checks if this location is a simple un-referenced constant (immediate) value
			constexpr bool is_immediate() const {
				return base == UNSET && index == UNSET && !reference /* TODO: check - what about immediate labels? */;
			}

			/// Checks if this location uses the index component
			constexpr bool is_indexed() const {
				return index.is(Registry::GENERAL);
			}

			/// Checks if this location is a simple un-referenced register
			constexpr bool is_simple() const {
				return base.is(Registry::GENERAL) && !is_indexed() && offset == 0 && !reference && !is_labeled();
			}

			/// Checks if this location is a simple un-referenced accumulator, used when encoding short-forms
			constexpr bool is_accum() const {
				return base.is(Registry::ACCUMULATOR) && is_simple();
			}

			/// Checks if this location requires label resolution
			constexpr bool is_labeled() const {
				return label != nullptr;
			}

			/// Checks if this location is a memory reference
			constexpr bool is_memory() const {
				return reference;
			}

			/// Checks if this location is a simple un-referenced register OR memory reference
			constexpr bool is_memreg() const {
				return is_memory() || is_simple();
			}

			/// Check if this a valid scaled register expression
			constexpr bool is_indexal() const {
				return (base.is(Registry::GENERAL) || base == UNSET) && (is_indexed() || index == UNSET);
			}

			/// Checks if this location is 'wide' (word, double word, or quad word)
			constexpr bool is_wide() const {
				return size == WORD || size == DWORD || size == QWORD;
			}

			/// Checks if this location is a floating-point register
			constexpr bool is_floating() const {
				return (base == ST) && !is_indexed() && !reference && !is_labeled() && (offset >= 0) && (offset <= 7);
			}

			/// Checks if this location is a floating-point register 'ST(0)'
			constexpr bool is_st0() const {
				return is_floating() && (offset == 0);
			}

			/// Checks if this location contains a label an nothing else
			constexpr bool is_jump_label() {
				return is_labeled() && base == UNSET && index == UNSET && !reference;
			}

			constexpr uint8_t get_mod_flag() const {
				if (label != nullptr) return MOD_QUAD;
				if (offset == 0) return MOD_NONE;
				if (util::min_bytes(offset) == BYTE) return MOD_BYTE;
				return MOD_QUAD;
			}

			constexpr uint8_t get_ss_flag() const {
				// count trailing zeros
				// 0001 -> 0, 0010 -> 1
				// 0100 -> 2, 1000 -> 3

				return __builtin_ctz(scale);
			}

	};

	/*
	 * Operators
	 */

	constexpr Location operator + (Registry registry, int offset) {
		return Location {registry, UNSET, 1, offset, nullptr, registry.size, false};
	}

	constexpr Location operator - (Registry registry, int offset) {
		return Location {registry, UNSET, 1, -offset, nullptr, registry.size, false};
	}

	constexpr Location operator + (Registry registry, const Label& label) {
		return Location {registry, UNSET, 1, 0, label.c_str(), registry.size, false};
	}

	constexpr Location operator + (Registry base, ScaledRegistry index) {
		uint8_t size = base.size;

		if (index.registry.size != VOID) {
			size = index.registry.size;
		}

		// TODO this is already checked in put_inst_std, but maybe we should check it here while we're at it?
		// if (size != VOID && index.registry.size != size) {
		// 	throw std::runtime_error {"Invalid operand, base and index registers need to be of the same size"};
		// }

		return Location {base, index.registry, index.scale, 0, nullptr, size, false};
	}

	constexpr Location operator + (ScaledRegistry index, int offset) {
		return Location {index} + offset;
	}

	constexpr Location operator - (ScaledRegistry index, int offset) {
		return Location {index} - offset;
	}

	constexpr Location operator + (ScaledRegistry index, const Label& label) {
		return Location {index} + label.c_str();
	}

	constexpr Location operator + (Registry base, Registry index) {
		return base + (index * 1);
	}

	template<uint8_t size_hint>
	Location cast(Location location) {
		return location.cast(size_hint);
	}

	template<uint8_t size_hint = VOID>
	Location ref(Location location) {
		return location.ref().cast(size_hint);
	}

	/// Deduce the operand size from a operand pair, also perform limited error checking
	uint8_t pair_size(const Location& lhs, const Location& rhs);

}