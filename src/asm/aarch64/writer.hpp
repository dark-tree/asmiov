#pragma once
#include <out/buffer/writer.hpp>

#include "argument/sizing.hpp"
#include "argument/registry.hpp"
#include "argument/shift.hpp"
#include "argument/condition.hpp"
#include "argument/sign.hpp"

namespace asmio::arm {

	class BufferWriter : public BasicBufferWriter {

		private:

			enum MemoryOperation {
				POST = 0b01,
				PRE = 0b11,
				OFFSET = 0b00,
			};

			/**
			 * Try to compute the ARM immediate bitmask based on a target value, this is a multi-step process
			 * that tries to guess the correct coefficients, if that fails it returns a std::nullopt.
			 *
			 * @param value The value to encode
			 * @param wide If the bitmask is expected to be used in a "wide" (64 bit) context
			 *
			 * @return Combined 'N:immr:imms' bit pattern, or nullopt.
			 */
			std::optional<uint16_t> compute_immediate_bitmask(uint64_t value, bool wide);

			/**
			 * Try to compute the ARM immediate bitmask based on a target value, this is a multi-step process
			 * that tries to guess the correct coefficients, if that fails it returns a std::nullopt.
			 *
			 * @param value The bit pattern to try encoding
			 * @param size Size of one repeating element within the pattern
			 *
			 * @return Combined 'N:immr:imms' bit pattern, or nullopt.
			 */
			std::optional<uint16_t> compute_element_bitmask(uint64_t value, size_t size);

			/**
			 * Constructs the N:immr:imms value for bitmask immediate instructions,
			 * the correctness of provided arguments IS NOT CHECKED.
			 *
			 * @param size size of one element in bits (2,4,8,16,32,64)
			 * @param ones number of ones in the element (1 <= ones <= size)
			 * @param roll right-roll of the elements within one pattern element (roll < size)
			 *
			 * @return Combined 'N:immr:imms' bit pattern
			 */
			uint16_t pack_bitmask(uint32_t size, uint32_t ones, uint32_t roll);

			/**
			 * Writes a standard bitmask immediate instruction into the buffer,
			 * with 'sf' derived from destination size.
			 *
			 * @param opc_from_23 Opcode (bit 23)
			 * @param destination Destination register (bit 0)
			 * @param source Source register (bit 5)
			 * @param n_immr_imms Packed immediate value, obtained from compute_immediate_bitmask() (bit 10)
			 */
			void put_inst_bitmask(uint32_t opc_from_23, Registry destination, Registry source, uint16_t n_immr_imms);

		private:

			/// Helper function used by some "link_*" types
			static void encode_shifted_aligned_link(SegmentedBuffer* buffer, const Linkage& linkage, int bits, int left_shift);

			static void link_26_0_aligned(SegmentedBuffer* buffer, const Linkage& linkage, size_t mount);
			static void link_19_5_aligned(SegmentedBuffer* buffer, const Linkage& linkage, size_t mount);
			static void link_21_5_lo_hi(SegmentedBuffer* buffer, const Linkage& linkage, size_t mount);

		private:

			static uint8_t pack_shift(uint8_t shift, bool wide);
			static uint64_t get_size(Size size);
			void assert_register_triplet(Registry a, Registry b, Registry c);

			/// Encode generic, 16 bit, immediate move, used by MOVN, MOVK, MOVZ
			void put_inst_mov(Registry registry, uint16_t opc, uint16_t imm, uint16_t shift);

			/// Encode ORR instruction, using the given N:R:S fields
			void put_inst_orr_bitmask(Registry destination, Registry source, uint16_t n_immr_imms);

			/// Encode "ADD/ADDS (extended register)" operation
			void put_inst_add_extended(Registry destination, Registry a, Registry b, Sizing add, uint8_t imm3, bool set_flags);

			/// Encode "ADC/ADCS (extended register)" operation
			void put_inst_adc(Registry destination, Registry a, Registry b, bool set_flags);

			/// Encode "CLS/CLZ" operation
			void put_inst_count(Registry destination, Registry source, uint8_t imm1);

			/// Encode "ILDR/LDRI/LDR" operation
			void put_inst_ldr(Registry dst, Registry base, int64_t offset, Sizing sizing, MemoryOperation op);

		public:

			void put_inst_orr(Registry destination, Registry a, Registry b, ShiftType shift = ShiftType::LSL, uint8_t imm6 = 0) {
				assert_register_triplet(a, b, destination);

				// if any one of them is a stack register abort
				if (!a.is(Registry::GENERAL) || !b.is(Registry::GENERAL) || !destination.is(Registry::GENERAL)) {
					throw std::runtime_error {"Invalid operands, expected general purpose registers."};
				}

				uint16_t sf = destination.wide() ? 1 : 0;
				put_dword(sf << 31 | 0b0101010 << 24 | uint8_t(shift) << 22 | a.reg << 16 | imm6 << 10 | b.reg << 5 | destination.reg);
			}

			void put_inst_orr(Registry destination, Registry source, uint64_t pattern) {
				const auto bitmask = compute_immediate_bitmask(pattern, destination.wide());

				if (!bitmask.has_value()) {
					throw std::runtime_error {"Invalid operands, the given constant is not encodable."};
				}

				put_inst_orr_bitmask(destination, source, bitmask.value());
			}

			void put_inst_add_imm(Registry destination, Registry source, uint16_t imm12, bool lsl_12 = false, bool set_flags = false) {

				if (source.is(Registry::ZERO) || destination.is(Registry::ZERO)) {
					throw std::runtime_error {"Invalid operands, zero register can't be used here."};
				}

				if (destination.wide() != source.wide()) {
					throw std::runtime_error {"Invalid operands, all given registers need to be of the same width."};
				}

				uint16_t sf = destination.wide() ? 1 : 0;
				uint32_t fb = (set_flags ? 1 : 0) << 29; // S bit
				put_dword(sf << 31 | 0b0'0'10001 << 24 | fb | (lsl_12 ? 0b01 : 0x00) << 22 | imm12 << 10 | source.reg << 5 | destination.reg);
			}

			void put_inst_add_shifted(Registry destination, Registry a, Registry b, ShiftType shift, uint8_t imm6, bool set_flags = false) {
				assert_register_triplet(a, b, destination);

				if (shift == ShiftType::ROR) {
					throw std::runtime_error {"Invalid shift type, ROR shift type is not allowed here."};
				}

				uint32_t sf = destination.wide() ? 1 : 0;
				uint32_t fb = (set_flags ? 1 : 0) << 29; // S bit
				put_dword(sf << 31 | 0b0'0'01011 << 24 | fb | uint8_t(shift) << 22 | b.reg << 16 | imm6 << 10 | a.reg << 5 | destination.reg);
			}

		public:

			BufferWriter(SegmentedBuffer& buffer);

			// basic
			INST put_adc(Registry dst, Registry a, Registry b);            /// Add with carry
			INST put_adcs(Registry dst, Registry a, Registry b);           /// Add with carry and set flags
			INST put_add(Registry dst, Registry a, Registry b, Sizing size = Sizing::UX, uint8_t lsl3 = 0); /// Add two registers, potentially extending one of them
			INST put_adds(Registry dst, Registry a, Registry b, Sizing size = Sizing::UX, uint8_t lsl3 = 0); /// Add two registers, set the flags, potentially extending one of them
			INST put_adr(Registry destination, Label label);               /// Form a PC-relative address
			INST put_adrp(Registry destination, Label label);              /// Form a PC-page-relative address
			INST put_movz(Registry dst, uint16_t imm, uint16_t shift = 0); /// Move shifted WORD into register, zero other bits
			INST put_movk(Registry dst, uint16_t imm, uint16_t shift = 0); /// Move shifted WORD into register, keep other bits
			INST put_movn(Registry dst, uint16_t imm, uint16_t shift = 0); /// Move shifted WORD into register, zero other bits, then NOT the register
			INST put_mov(Registry dst, uint64_t imm);                      /// Move immediate into register
			INST put_mov(Registry dst, Registry src);                      /// Move value between registers
			INST put_nop();                                                /// No operation
			INST put_ret();                                                /// Return from procedure using link register
			INST put_ret(Registry src);                                    /// Return from procedure
			INST put_rbit(Registry dst, Registry src);                     /// Reverse bits
			INST put_clz(Registry dst, Registry src);                      /// Count leading zeros
			INST put_cls(Registry dst, Registry src);                      /// Count leading signs (ones)
			INST put_ldr(Registry registry, Label label);                  /// Load value from memory
			INST put_ildr(Registry dst, Registry base, int64_t offset, Sizing size); /// Increment base and load value from memory
			INST put_ldri(Registry dst, Registry base, int64_t offset, Sizing size); /// Load value from memory and increment base
			INST put_ldr(Registry registry, Registry base, uint64_t offset, Sizing size); /// Load value from memory

			// branch
			INST put_b(Label label);                                       /// Branch
			INST put_b(Condition condition, Label label);                  /// Branch conditionally
			INST put_bl(Label label);                                      /// Branch with link
			INST put_blr(Registry ptr);                                    /// Branch with link to register
			INST put_br(Registry ptr);                                     /// Branch to register
			INST put_cbnz(Registry src, Label label);                      /// Branch if register is not zero
			INST put_cbz(Registry src, Label label);                       /// Branch if register is zero


	};

}
