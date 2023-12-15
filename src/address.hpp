#pragma once

#include <stdexcept>
#include <bit>
#include "util.hpp"
#include "const.hpp"

#define ASSERT_MUTABLE(loc) if ((loc).reference) throw std::runtime_error {"Reference can't be modified!"};

namespace asmio::x86 {

	constexpr const uint8_t VOID = 0;
	constexpr const uint8_t BYTE = 1;
	constexpr const uint8_t WORD = 2;
	constexpr const uint8_t DWORD = 4;

	struct Registry {

		enum Flag {
			NONE    = 0b00,
			PSEUDO  = 0b01,
			HIGHER  = 0b10,
			GENERAL = 0100
		};

		enum struct Name : uint8_t {
			UNSET, A, B, C, D, SI, DI, BP, SP
		};

		const uint8_t size;
		const Name name;
		const uint8_t flag : 5;
		const uint8_t reg : 3;

		constexpr Registry(uint8_t size, uint8_t reg, Name name, uint8_t flag)
		: size(size), name(name), flag(flag), reg(reg) {}

		bool is(Registry::Flag mask) const {
			return (flag & mask) != 0;
		}

		bool is(Registry registry) const {
			return this->size == registry.size && this->name == registry.name && this->flag == registry.flag;
		}

		bool is_wide() const {
			return size != 1;
		}

	};

	constexpr const Registry UNSET {VOID,  0b000, Registry::Name::UNSET, Registry::PSEUDO};
	constexpr const Registry EAX   {DWORD, 0b000, Registry::Name::A, Registry::GENERAL};
	constexpr const Registry AX    {WORD,  0b000, Registry::Name::A, Registry::GENERAL};
	constexpr const Registry AL    {BYTE,  0b000, Registry::Name::A, Registry::GENERAL};
	constexpr const Registry AH    {BYTE,  0b100, Registry::Name::A, Registry::GENERAL | Registry::HIGHER};
	constexpr const Registry EBX   {DWORD, 0b011, Registry::Name::B, Registry::GENERAL};
	constexpr const Registry BX    {WORD,  0b011, Registry::Name::B, Registry::GENERAL};
	constexpr const Registry BL    {BYTE,  0b011, Registry::Name::B, Registry::GENERAL};
	constexpr const Registry BH    {BYTE,  0b111, Registry::Name::B, Registry::GENERAL | Registry::HIGHER};
	constexpr const Registry ECX   {DWORD, 0b001, Registry::Name::C, Registry::GENERAL};
	constexpr const Registry CX    {WORD,  0b001, Registry::Name::C, Registry::GENERAL};
	constexpr const Registry CL    {BYTE,  0b001, Registry::Name::C, Registry::GENERAL};
	constexpr const Registry CH    {BYTE,  0b101, Registry::Name::C, Registry::GENERAL | Registry::HIGHER};
	constexpr const Registry EDX   {DWORD, 0b010, Registry::Name::D, Registry::GENERAL};
	constexpr const Registry DX    {WORD,  0b010, Registry::Name::D, Registry::GENERAL};
	constexpr const Registry DL    {BYTE,  0b010, Registry::Name::D, Registry::GENERAL};
	constexpr const Registry DH    {BYTE,  0b110, Registry::Name::D, Registry::GENERAL | Registry::HIGHER};
	constexpr const Registry ESI   {DWORD, 0b110, Registry::Name::SI, Registry::NONE};
	constexpr const Registry SI    {WORD,  0b110, Registry::Name::SI, Registry::NONE};
	constexpr const Registry EDI   {DWORD, 0b111, Registry::Name::DI, Registry::NONE};
	constexpr const Registry DI    {WORD,  0b111, Registry::Name::DI, Registry::NONE};
	constexpr const Registry EBP   {DWORD, 0b101, Registry::Name::BP, Registry::NONE};
	constexpr const Registry BP    {WORD,  0b101, Registry::Name::BP, Registry::NONE};
	constexpr const Registry ESP   {DWORD, 0b100, Registry::Name::SP, Registry::NONE};
	constexpr const Registry SP    {WORD,  0b100, Registry::Name::SP, Registry::NONE};

	void assertValidScale(Registry registry, uint8_t multiplier) {
		if (registry.name == Registry::Name::SP && registry.size == DWORD) {
			throw std::runtime_error {"The ESP registry can't be scaled!"};
		}

		if (std::popcount(multiplier) != 1 || (multiplier & 0b1111) == 0) {
			throw std::runtime_error {"A registry can only be scaled by one of (1, 2, 4, 8) in expression!"};
		}
	}

	struct ScaledRegistry {

		public:

			const Registry registry;
			const uint8_t scale;

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
			const uint32_t scale;
			const int offset;
			const bool reference;
			const uint8_t size;

		public:

			Location(int offset = 0)
			: Location(UNSET, UNSET, 1, offset, DWORD, false) {}

			Location(Registry registry)
			: Location(registry, UNSET, 1, 0, registry.size, false) {}

			Location(ScaledRegistry index)
			: Location(UNSET, index.registry, index.scale, 0, DWORD, false) {}

			explicit Location(Registry base, Registry index, uint32_t scale, int offset, int size, bool reference)
			: base(base), index(index), scale(scale), offset(offset), size(size), reference(reference) {
				assertValidScale(index, scale);
			}

		public:

			Location ref(uint8_t size_hint) {
				ASSERT_MUTABLE(*this);
				return Location {base, index, scale, offset, size_hint, true};
			}

			Location operator + (int extend) const {
				ASSERT_MUTABLE(*this);
				return Location {base, index, scale, offset + extend, size, false};
			}

			Location operator - (int extend) const {
				ASSERT_MUTABLE(*this);
				return Location {base, index, scale, offset - extend, size, false};
			}

			/**
			 * Checks if this location is a simple
			 * un-referenced constant (immediate) value
			 */
			bool is_immediate() {
				return base.is(UNSET) && index.is(UNSET) && scale == 1 && !reference;
			}

			/**
			 * Checks if this location
			 * uses the index component
			 */
			bool is_indexed() {
				return !index.is(UNSET);
			}

			/**
			 * Checks if this location is a simple
			 * un-referenced register
			 */
			bool is_simple() {
				return !base.is(UNSET) && !is_indexed() && offset == 0 && !reference;
			}


			bool is_memory() {
				return (reference && !base.is(UNSET)) || is_simple();
			}

			uint8_t get_mod_flag() {
				if (offset == 0) return MOD_NONE;
				if (min_bytes(offset) == BYTE) return MOD_BYTE;
				return MOD_QUAD;
			}

			uint8_t get_ss_flag() {
				// count trailing zeros
				// 0001 -> 0, 0010 -> 1
				// 0100 -> 2, 1000 -> 3

				return __builtin_ctz(scale);
			}

	};

	Location operator + (Registry registry, int offset) {
		return Location {registry, UNSET, 1, offset, DWORD, false};
	}

	ScaledRegistry operator * (Registry registry, uint8_t scale) {
		return ScaledRegistry {registry, scale};
	}

	Location operator + (Registry base, ScaledRegistry index) {
		return Location {base, index.registry, index.scale, 0, DWORD, false};
	}

	Location operator + (ScaledRegistry index, int offset) {
		return Location {index} + offset;
	}

	Location operator + (Registry base, Registry index) {
		return base + (index * 1);
	}

	template<uint8_t size_hint = DWORD>
	Location ref(Location location) {
		return location.ref(size_hint);
	}

}

