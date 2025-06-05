#pragma once

#include "external.hpp"
#include "util.hpp"
#include "const.hpp"
#include "label.hpp"

namespace asmio::x86 {

	constexpr uint8_t VOID = 0;
	constexpr uint8_t BYTE = 1;
	constexpr uint8_t WORD = 2;
	constexpr uint8_t DWORD = 4;
	constexpr uint8_t QWORD = 8;
	constexpr uint8_t TWORD = 10;

	struct RegInfo {
		uint8_t rex; // REX prefix is needed to encode this register
		uint8_t reg;

		RegInfo(bool rex, uint8_t reg) {
			this->rex = rex;
			this->reg = reg;
		}

		static RegInfo raw(uint8_t reg) {
			return {false, reg};
		}

		/// Return the lower 3 bit of the register number
		uint8_t low() const {
			return reg & REG_LOW;
		}

		/// Checks if REX.R prefix is needed
		bool is_extended() const {
			return reg & REG_HIGH;
		}
	};

	struct __attribute__((__packed__)) Registry {

		// a bit useless rn
		enum Flag : uint8_t {
			NONE        = 0b00000,

			GENERAL     = 0b0000001,
			FLOATING    = 0b0000010,
			ACCUMULATOR = 0b0000100,
			REX         = 0b0001000, // this registers requires the REX prefix to be encoded
			HIGH_BYTE   = 0b0010000, // registers that CANT be used with REX prefix present
		};

		const uint8_t size; // size in bytes
		const uint8_t flag; // additional flags
		const uint8_t reg;  // registry x86 code

		constexpr Registry(uint8_t size, uint8_t reg, uint8_t flag)
		: size(size), flag(flag), reg(reg) {}

		bool operator==(Registry const& other) const {
			return is(other);
		}

		bool is(Flag mask) const {
			return (flag & mask) != 0;
		}

		bool is(Registry registry) const {
			return this->size == registry.size && this->reg == registry.reg && this->flag == registry.flag;
		}

		RegInfo pack() const {
			return {is(REX), reg};
		}

		uint8_t low() const {
			return reg & 0b0111;
		}

		bool is_esp_like() const {
			return low() == 0b100;
		}

		bool is_ebp_like() const {
			return low() == 0b101;
		}

	};

	// 386
	constexpr Registry UNSET {VOID,  0b0000, Registry::NONE};
	constexpr Registry EAX   {DWORD, 0b0000, Registry::GENERAL | Registry::ACCUMULATOR};
	constexpr Registry AX    {WORD,  0b0000, Registry::GENERAL | Registry::ACCUMULATOR};
	constexpr Registry AL    {BYTE,  0b0000, Registry::GENERAL | Registry::ACCUMULATOR};
	constexpr Registry AH    {BYTE,  0b0100, Registry::GENERAL | Registry::HIGH_BYTE};
	constexpr Registry EBX   {DWORD, 0b0011, Registry::GENERAL};
	constexpr Registry BX    {WORD,  0b0011, Registry::GENERAL};
	constexpr Registry BL    {BYTE,  0b0011, Registry::GENERAL};
	constexpr Registry BH    {BYTE,  0b0111, Registry::GENERAL | Registry::HIGH_BYTE};
	constexpr Registry ECX   {DWORD, 0b0001, Registry::GENERAL};
	constexpr Registry CX    {WORD,  0b0001, Registry::GENERAL};
	constexpr Registry CL    {BYTE,  0b0001, Registry::GENERAL};
	constexpr Registry CH    {BYTE,  0b0101, Registry::GENERAL | Registry::HIGH_BYTE};
	constexpr Registry EDX   {DWORD, 0b0010, Registry::GENERAL};
	constexpr Registry DX    {WORD,  0b0010, Registry::GENERAL};
	constexpr Registry DL    {BYTE,  0b0010, Registry::GENERAL};
	constexpr Registry DH    {BYTE,  0b0110, Registry::GENERAL | Registry::HIGH_BYTE};
	constexpr Registry ESI   {DWORD, 0b0110, Registry::GENERAL};
	constexpr Registry SI    {WORD,  0b0110, Registry::GENERAL};
	constexpr Registry EDI   {DWORD, 0b0111, Registry::GENERAL};
	constexpr Registry DI    {WORD,  0b0111, Registry::GENERAL};
	constexpr Registry EBP   {DWORD, 0b0101, Registry::GENERAL};
	constexpr Registry BP    {WORD,  0b0101, Registry::GENERAL};
	constexpr Registry ESP   {DWORD, 0b0100, Registry::GENERAL};
	constexpr Registry SP    {WORD,  0b0100, Registry::GENERAL};
	constexpr Registry ST    {TWORD, 0b0000, Registry::FLOATING};

	// amd64 surrogates - uniform byte registers
	constexpr Registry SPL   {BYTE,  AH.reg, Registry::GENERAL | Registry::REX};
	constexpr Registry BPL   {BYTE,  CH.reg, Registry::GENERAL | Registry::REX};
	constexpr Registry SIL   {BYTE,  DH.reg, Registry::GENERAL | Registry::REX};
	constexpr Registry DIL   {BYTE,  BH.reg, Registry::GENERAL | Registry::REX};

	// amd64
	constexpr Registry RAX   {QWORD, 0b0000, Registry::GENERAL | Registry::ACCUMULATOR | Registry::REX};
	constexpr Registry RBX   {QWORD, 0b0011, Registry::GENERAL | Registry::REX};
	constexpr Registry RCX   {QWORD, 0b0001, Registry::GENERAL | Registry::REX};
	constexpr Registry RDX   {QWORD, 0b0010, Registry::GENERAL | Registry::REX};
	constexpr Registry RSI   {QWORD, 0b0110, Registry::GENERAL | Registry::REX};
	constexpr Registry RDI   {QWORD, 0b0111, Registry::GENERAL | Registry::REX};
	constexpr Registry RBP   {QWORD, 0b0101, Registry::GENERAL | Registry::REX};
	constexpr Registry RSP   {QWORD, 0b0100, Registry::GENERAL | Registry::REX};
	constexpr Registry R8L   {BYTE,  0b1000, Registry::GENERAL | Registry::REX};
	constexpr Registry R8W   {WORD,  0b1000, Registry::GENERAL | Registry::REX};
	constexpr Registry R8D   {DWORD, 0b1000, Registry::GENERAL | Registry::REX};
	constexpr Registry R8    {QWORD, 0b1000, Registry::GENERAL | Registry::REX};
	constexpr Registry R9L   {BYTE,  0b1001, Registry::GENERAL | Registry::REX};
	constexpr Registry R9W   {WORD,  0b1001, Registry::GENERAL | Registry::REX};
	constexpr Registry R9D   {DWORD, 0b1001, Registry::GENERAL | Registry::REX};
	constexpr Registry R9    {QWORD, 0b1001, Registry::GENERAL | Registry::REX};
	constexpr Registry R10L  {BYTE,  0b1010, Registry::GENERAL | Registry::REX};
	constexpr Registry R10W  {WORD,  0b1010, Registry::GENERAL | Registry::REX};
	constexpr Registry R10D  {DWORD, 0b1010, Registry::GENERAL | Registry::REX};
	constexpr Registry R10   {QWORD, 0b1010, Registry::GENERAL | Registry::REX};
	constexpr Registry R11L  {BYTE,  0b1011, Registry::GENERAL | Registry::REX};
	constexpr Registry R11W  {WORD,  0b1011, Registry::GENERAL | Registry::REX};
	constexpr Registry R11D  {DWORD, 0b1011, Registry::GENERAL | Registry::REX};
	constexpr Registry R11   {QWORD, 0b1011, Registry::GENERAL | Registry::REX};
	constexpr Registry R12L  {BYTE,  0b1100, Registry::GENERAL | Registry::REX};
	constexpr Registry R12W  {WORD,  0b1100, Registry::GENERAL | Registry::REX};
	constexpr Registry R12D  {DWORD, 0b1100, Registry::GENERAL | Registry::REX};
	constexpr Registry R12   {QWORD, 0b1100, Registry::GENERAL | Registry::REX};
	constexpr Registry R13L  {BYTE,  0b1101, Registry::GENERAL | Registry::REX};
	constexpr Registry R13W  {WORD,  0b1101, Registry::GENERAL | Registry::REX};
	constexpr Registry R13D  {DWORD, 0b1101, Registry::GENERAL | Registry::REX};
	constexpr Registry R13   {QWORD, 0b1101, Registry::GENERAL | Registry::REX};
	constexpr Registry R14L  {BYTE,  0b1110, Registry::GENERAL | Registry::REX};
	constexpr Registry R14W  {WORD,  0b1110, Registry::GENERAL | Registry::REX};
	constexpr Registry R14D  {DWORD, 0b1110, Registry::GENERAL | Registry::REX};
	constexpr Registry R14   {QWORD, 0b1110, Registry::GENERAL | Registry::REX};
	constexpr Registry R15L  {BYTE,  0b1111, Registry::GENERAL | Registry::REX};
	constexpr Registry R15W  {WORD,  0b1111, Registry::GENERAL | Registry::REX};
	constexpr Registry R15D  {DWORD, 0b1111, Registry::GENERAL | Registry::REX};
	constexpr Registry R15   {QWORD, 0b1111, Registry::GENERAL | Registry::REX};


	void assertValidScale(Registry registry, uint8_t multiplier);
	void assertNonReferential(class Location const* location, const char* why);

	struct ScaledRegistry {

		public:

			const Registry registry;
			const uint8_t scale : 4; // the scale, as integer

		public:

			explicit ScaledRegistry(Registry registry, uint8_t scale)
			: registry(registry), scale(scale) {
				assertValidScale(registry, scale);
			}

			bool operator==(ScaledRegistry const& other) const {
				return (registry == other.registry) && (scale == other.scale);
			}

	};

	class Location {

		public:

			const Registry base;
			const Registry index;
			const uint8_t scale  : 4; // the scale, as integer
			const bool reference : 4; // is this location a memory reference
			const uint8_t size;
			const int64_t offset;
			const char* label;

		public:

			template<typename T>
			Location(T offset = 0) // can the same thing be archived with some smart overload?
			: Location(UNSET, UNSET, 1, util::get_int_or(offset), util::get_ptr_or(offset), VOID, false) {}

			Location(Registry registry)
			: Location(registry, UNSET, 1, 0, nullptr, registry.size, false) {}

			Location(ScaledRegistry index)
			: Location(UNSET, index.registry, index.scale, 0, nullptr, index.registry.size, false) {}

			explicit Location(Registry base, Registry index, uint32_t scale, int64_t offset, const char* label, int size, bool reference)
			: base(base), index(index), scale(scale), offset(offset), label(label), size(size), reference(reference) {
				assertValidScale(index, scale);
			}

		public:

			Location ref() const {
				assertNonReferential(this, "Can't reference a reference!");
				return Location {base, index, scale, offset, label, VOID, true};
			}

			Location cast(uint8_t bytes) const {
				if (reference || is_immediate()) {
					return Location {base, index, scale, offset, label, bytes, reference};
				}

				throw std::runtime_error {"The result of this expression is of fixed size!"};
			}

			Location operator + (int64_t extend) const {
				assertNonReferential(this, "Can't modify a reference!");
				return Location {base, index, scale, offset + extend, label, size, false};
			}

			Location operator - (int64_t extend) const {
				assertNonReferential(this, "Can't modify a reference!");
				return Location {base, index, scale, offset - extend, label, size, false};
			}

			Location operator + (const char* label) const {
				assertNonReferential(this, "Can't modify a reference!");
				return Location {base, index, scale, offset, label, size, false};
			}

			/// Check if the size of this element hasn't yet been defined
			bool is_indeterminate() const {
				return size == VOID;
			}

			/// Checks if this location is a simple un-referenced constant (immediate) value
			bool is_immediate() const {
				return base.is(UNSET) && index.is(UNSET) && !reference /* TODO: check - what about immediate labels? */;
			}

			/// Checks if this location uses the index component
			bool is_indexed() const {
				return index.is(Registry::GENERAL);
			}

			/// Checks if this location is a simple un-referenced register
			bool is_simple() const {
				return base.is(Registry::GENERAL) && !is_indexed() && offset == 0 && !reference && !is_labeled();
			}

			/// Checks if this location is a simple un-referenced accumulator, used when encoding short-forms
			bool is_accum() const {
				return base.is(Registry::ACCUMULATOR) && is_simple();
			}

			/// Checks if this location requires label resolution
			bool is_labeled() const {
				return label != nullptr;
			}

			/// Checks if this location is a memory reference
			bool is_memory() const {
				return reference;
			}

			/// Checks if this location is a simple un-referenced register OR memory reference
			bool is_memreg() const {
				return is_memory() || is_simple();
			}

			/// Check if this a valid scaled register expression
			bool is_indexal() const {
				return (base.is(Registry::GENERAL) || base.is(UNSET)) && (is_indexed() || index.is(UNSET));
			}

			/// Checks if this location is 'wide' (word, double word, or quad word)
			bool is_wide() const {
				return size == WORD || size == DWORD || size == QWORD;
			}

			/// Checks if this location is a floating-point register
			bool is_floating() const {
				return base.is(ST) && !is_indexed() && !reference && !is_labeled() && (offset >= 0) && (offset <= 7);
			}

			/// Checks if this location is a floating-point register 'ST(0)'
			bool is_st0() const {
				return is_floating() && (offset == 0);
			}

			/// Checks if this location contains a label an nothing else
			bool is_jump_label() {
				return is_labeled() && base.is(UNSET) && index.is(UNSET) && !reference;
			}

			uint8_t get_mod_flag() const {
				if (label != nullptr) return MOD_QUAD;
				if (offset == 0) return MOD_NONE;
				if (util::min_bytes(offset) == BYTE) return MOD_BYTE;
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
	Location operator - (Registry registry, int offset);
	Location operator + (Registry registry, Label label);
	ScaledRegistry operator * (Registry registry, uint8_t scale);
	Location operator + (Registry base, ScaledRegistry index);
	Location operator + (ScaledRegistry index, int offset);
	Location operator - (ScaledRegistry index, int offset);
	Location operator + (ScaledRegistry index, Label label);
	Location operator + (Registry base, Registry index);

	/// investigate if immediate size ever matters, if not return to the old system
	template<uint8_t size_hint>
	Location cast(Location location) {
		return location.cast(size_hint);
	}

	template<uint8_t size_hint = VOID>
	Location ref(Location location) {
		return location.ref().cast(size_hint);
	}


	uint8_t pair_size(const Location& a, const Location& b);

}

