#pragma once

#include "external.hpp"
#include "asm/x86/const.hpp"
#include "out/buffer/sizes.hpp"
#include "asm/util.hpp"
#include <macro.hpp>

namespace asmio::x86 {

	/// Represents the MODRM.reg field, a simplified view of a full location or a custom code
	struct RegInfo {

		const uint8_t rex; // REX prefix is needed to encode this register
		const uint8_t reg;

		/// Avoid calling yourself, user Registry::pack() instead
		constexpr RegInfo(bool rex, uint8_t reg)
			: rex(rex), reg(reg) {
		}

		/// Construct RegInfo from raw data, useful for instructions that pack the opcode into MODRM.reg
		constexpr static RegInfo raw(uint8_t reg) {
			return {false, reg};
		}

		/// Return the lower 3 bit of the register number
		constexpr uint8_t low() const {
			return reg & REG_LOW;
		}

		/// Checks if REX.R prefix is needed
		constexpr bool is_extended() const {
			return reg & REG_HIGH;
		}

	};

	/// Represents a x86 CPU registry
	struct PACKED Registry {

		/// Register properties
		enum Flag : uint8_t {
			NONE        = 0b0000000,

			GENERAL     = 0b0000001,
			FLOATING    = 0b0000010,
			ACCUMULATOR = 0b0000100, // this is the accumulator (RAX/EAX/AX)
			REX         = 0b0001000, // this registers requires the REX prefix to be encoded
			HIGH_BYTE   = 0b0010000, // registers that CANT be used with REX prefix present
		};

		const uint8_t size; // size in bytes
		const uint8_t flag; // additional flags
		const uint8_t reg;  // registry x86 code

		constexpr Registry(uint8_t size, uint8_t reg, uint8_t flag)
		: size(size), flag(flag), reg(reg) {}

		/// Check is two registers are EXACTLY the same register
		constexpr bool operator == (Registry const& other) const {
			return this->size == other.size && this->reg == other.reg && this->flag == other.flag;
		}

		/// Check if the register has a given flag
		constexpr bool is(Flag mask) const {
			return (flag & mask) != 0;
		}

		/// Convert Registry into a RegInfo structure
		constexpr RegInfo pack() const {
			return {is(REX), reg};
		}

		/// Retrieve the lower bits of the registry number
		constexpr uint8_t low() const {
			return reg & REG_LOW;
		}

		/// Retrieve the REX high bit of the registry number
		constexpr uint8_t high() const {
			return reg & REG_HIGH;
		}

		/// Check if the register has ESP-like encoding quirks
		constexpr bool is_esp_like() const {
			return low() == 0b100;
		}

		/// Check if the register has EBP-like encoding quirks
		constexpr bool is_ebp_like() const {
			return low() == 0b101;
		}

	};

	/*
	 * i386
	 */

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

	/*
	 * Amd64 surrogates - uniform byte registers
	 */

	constexpr Registry SPL   {BYTE,  AH.reg, Registry::GENERAL | Registry::REX};
	constexpr Registry BPL   {BYTE,  CH.reg, Registry::GENERAL | Registry::REX};
	constexpr Registry SIL   {BYTE,  DH.reg, Registry::GENERAL | Registry::REX};
	constexpr Registry DIL   {BYTE,  BH.reg, Registry::GENERAL | Registry::REX};

	/*
	 * Amd64
	 */

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

}