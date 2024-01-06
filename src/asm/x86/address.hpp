#pragma once

#include "external.hpp"
#include "util.hpp"
#include "const.hpp"
#include "label.hpp"

namespace asmio::x86 {

	constexpr const uint8_t VOID = 0;
	constexpr const uint8_t BYTE = 1;
	constexpr const uint8_t WORD = 2;
	constexpr const uint8_t DWORD = 4;
	constexpr const uint8_t QWORD = 8;
	constexpr const uint8_t TWORD = 10;

	struct __attribute__((__packed__)) Registry {

		// a bit useless rn
		enum Flag {
			NONE     = 0b000,
			PSEUDO   = 0b001,
			GENERAL  = 0b010,
			FLOATING = 0b100
		};

		const uint8_t size : 8; // size in bytes
		const uint8_t flag : 5; // additional flags
		const uint8_t reg  : 3; // registry x86 code

		constexpr Registry(uint8_t size, uint8_t reg, uint8_t flag)
		: size(size), flag(flag), reg(reg) {}

		bool is(Registry::Flag mask) const {
			return (flag & mask) != 0;
		}

		bool is(Registry registry) const {
			return this->size == registry.size && this->reg == registry.reg && this->flag == registry.flag;
		}

	};

	constexpr const Registry UNSET {VOID,  0b000, Registry::PSEUDO};
	constexpr const Registry EAX   {DWORD, 0b000, Registry::GENERAL};
	constexpr const Registry AX    {WORD,  0b000, Registry::GENERAL};
	constexpr const Registry AL    {BYTE,  0b000, Registry::GENERAL};
	constexpr const Registry AH    {BYTE,  0b100, Registry::GENERAL};
	constexpr const Registry EBX   {DWORD, 0b011, Registry::GENERAL};
	constexpr const Registry BX    {WORD,  0b011, Registry::GENERAL};
	constexpr const Registry BL    {BYTE,  0b011, Registry::GENERAL};
	constexpr const Registry BH    {BYTE,  0b111, Registry::GENERAL};
	constexpr const Registry ECX   {DWORD, 0b001, Registry::GENERAL};
	constexpr const Registry CX    {WORD,  0b001, Registry::GENERAL};
	constexpr const Registry CL    {BYTE,  0b001, Registry::GENERAL};
	constexpr const Registry CH    {BYTE,  0b101, Registry::GENERAL};
	constexpr const Registry EDX   {DWORD, 0b010, Registry::GENERAL};
	constexpr const Registry DX    {WORD,  0b010, Registry::GENERAL};
	constexpr const Registry DL    {BYTE,  0b010, Registry::GENERAL};
	constexpr const Registry DH    {BYTE,  0b110, Registry::GENERAL};
	constexpr const Registry ESI   {DWORD, 0b110, Registry::GENERAL};
	constexpr const Registry SI    {WORD,  0b110, Registry::GENERAL};
	constexpr const Registry EDI   {DWORD, 0b111, Registry::GENERAL};
	constexpr const Registry DI    {WORD,  0b111, Registry::GENERAL};
	constexpr const Registry EBP   {DWORD, 0b101, Registry::GENERAL};
	constexpr const Registry BP    {WORD,  0b101, Registry::GENERAL};
	constexpr const Registry ESP   {DWORD, 0b100, Registry::GENERAL};
	constexpr const Registry SP    {WORD,  0b100, Registry::GENERAL};
	constexpr const Registry ST    {TWORD, 0b000, Registry::FLOATING};

	void assertValidScale(Registry registry, uint8_t multiplier);
	void assertNonReferencial(class Location const* location, const char* why);

	struct ScaledRegistry {

		public:

			const Registry registry;
			const uint8_t scale : 4; // the scale, as integer

		public:

			explicit ScaledRegistry(Registry registry, uint8_t scale)
			: registry(registry), scale(scale) {
				assertValidScale(registry, scale);
			}

	};

	class Location {

		public:

			const Registry base;
			const Registry index;
			const uint8_t scale  : 4; // the scale, as integer
			const bool reference : 4; // is this location a memory reference
			const uint8_t size;
			const long offset;
			const char* label;

		public:

			template<typename T>
			Location(T offset = 0) // can the same thing be archived with some smart overload?
			: Location(UNSET, UNSET, 1, get_int_or(offset), get_ptr_or(offset), DWORD, false) {}

			Location(Registry registry)
			: Location(registry, UNSET, 1, 0, nullptr, registry.size, false) {}

			Location(ScaledRegistry index)
			: Location(UNSET, index.registry, index.scale, 0, nullptr, DWORD, false) {}

			explicit Location(Registry base, Registry index, uint32_t scale, long offset, const char* label, int size, bool reference)
			: base(base), index(index), scale(scale), offset(offset), label(label), size(size), reference(reference) {
				assertValidScale(index, scale);
			}

		public:

			Location ref() const {
				assertNonReferencial(this, "Can't reference a reference!");
				return Location {base, index, scale, offset, label, size, true};
			}

			Location cast(uint8_t bytes) const {
				if (reference || is_immediate()) {
					return Location {base, index, scale, offset, label, bytes, true};
				}

				throw std::runtime_error {"The result of this expression is of fixed size!"};
			}

			Location operator + (int extend) const {
				assertNonReferencial(this, "Can't modify a reference!");
				return Location {base, index, scale, offset + extend, label, size, false};
			}

			Location operator - (int extend) const {
				assertNonReferencial(this, "Can't modify a reference!");
				return Location {base, index, scale, offset - extend, label, size, false};
			}

			Location operator + (const char* label) const {
				assertNonReferencial(this, "Can't modify a reference!");
				return Location {base, index, scale, offset, label, size, false};
			}

			/**
			 * Checks if this location is a simple un-referenced constant (immediate) value
			 */
			bool is_immediate() const {
				return base.is(UNSET) && index.is(UNSET) && scale == 1 && !reference /* TODO: check - what about immediate labels? */;
			}

			/**
			 * Checks if this location uses the index component
			 */
			bool is_indexed() const {
				return !index.is(UNSET);
			}

			/**
			 * Checks if this location is a simple un-referenced register
			 */
			bool is_simple() const {
				return (base.flag & Registry::GENERAL) && !is_indexed() && offset == 0 && !reference && !is_labeled();
			}

			/**
			 * Checks if this location requires label resolution
			 */
			bool is_labeled() const {
				return label != nullptr;
			}

			/**
			 * Checks if this location is a memory reference
			 */
			bool is_memory() const {
				return reference;
			}

			/**
			 * Checks if this location is a simple un-referenced register OR memory reference
			 */
			bool is_memreg() const {
				return is_memory() || is_simple();
			}

			/**
			 * Checks if this location is 'wide' (word or double word)
			 */
			bool is_wide() const {
				return size == 2 || size == 4;
			}

			/**
			 * Checks if this location is a floating-point register
			 */
			bool is_floating() const {
				return base.is(ST) && !is_indexed() && !reference && !is_labeled() && (offset >= 0) && (offset <= 7);
			}

			/**
			 * Checks if this location is a floating-point register 'ST(0)'
			 */
			bool is_st0() const {
				return is_floating() && (offset == 0);
			}

			/**
			* Checks if this location contains a label an nothing else
			*/
			bool is_jump_label() {
				return is_labeled() && base.is(UNSET) && index.is(UNSET) && !reference;
			}

			uint8_t get_mod_flag() const {
				if (label != nullptr) return MOD_QUAD;
				if (offset == 0) return MOD_NONE;
				if (min_bytes(offset) == BYTE) return MOD_BYTE;
				return MOD_QUAD;
			}

			uint8_t get_ss_flag() const {
				// count trailing zeros
				// 0001 -> 0, 0010 -> 1
				// 0100 -> 2, 1000 -> 3

				return __builtin_ctz(scale);
			}

	};

	Location operator + (Registry registry, int offset);
	Location operator + (Registry registry, Label label);
	ScaledRegistry operator * (Registry registry, uint8_t scale);
	Location operator + (Registry base, ScaledRegistry index);
	Location operator + (ScaledRegistry index, int offset);
	Location operator + (ScaledRegistry index, Label label);
	Location operator + (Registry base, Registry index);
	Location ref(Location location);

	/// investigate if immediate size ever matters, if not return to the old system
	template<uint8_t size_hint = DWORD>
	Location cast(Location location) {
		return location.cast(size_hint);
	}

}

