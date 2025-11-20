#pragma once
#include <out/buffer/writer.hpp>

#include "argument/sizing.hpp"
#include "argument/registry.hpp"
#include "argument/shift.hpp"
#include "argument/condition.hpp"
#include "argument/pattern.hpp"

namespace asmio::arm {

	class BufferWriter : public BasicBufferWriter {

		private:

			enum MemoryOperation : uint8_t {
				POST = 0b01,
				PRE = 0b11,
				OFFSET = 0b00,
			};

			enum MemoryDirection : uint8_t {
				LOAD = 0b11,
				STORE = 0b00,
			};

			/**
			 * Writes a standard 'bitmask immediate' instruction into the buffer,
			 * with 'sf' derived from destination size.
			 */
			void put_inst_bitmask_immediate(uint32_t opc_from_23, Registry destination, Registry source, BitPattern pattern);

			/**
			 * Writes the standard 'shifted register' instruction into the buffer,
			 * with 'sf' derived from destination size.
			 */
			void put_inst_shifted_register(uint32_t opc_from_24, uint32_t bit_21, Registry dst, Registry n, Registry m, uint8_t imm6, ShiftType shift);

			/**
			 * Writes the standard 'extended register' instruction into the buffer,
			 * with 'sf' derived from destination size. This command accepts SP as destination only when set_flags is false.
			 */
			void put_inst_extended_register(uint32_t opcode_from_21, Registry destination, Registry a, Registry b, Sizing add, uint8_t imm3, bool set_flags);

		private:

			/// Helper function used by some "link_*" types
			static void encode_shifted_aligned_link(SegmentedBuffer* buffer, const Linkage& linkage, int bits, int left_shift);

			static void link_26_0_aligned(SegmentedBuffer* buffer, const Linkage& linkage, size_t mount);
			static void link_14_5_aligned(SegmentedBuffer* buffer, const Linkage& linkage, size_t mount);
			static void link_19_5_aligned(SegmentedBuffer* buffer, const Linkage& linkage, size_t mount);
			static void link_21_5_lo_hi(SegmentedBuffer* buffer, const Linkage& linkage, size_t mount);

		protected:

			static uint8_t pack_shift(uint8_t shift, bool wide);
			static uint64_t get_size(Size size);
			void assert_register_triplet(Registry a, Registry b, Registry c);

			/// Encode generic, 16 bit, immediate move, used by MOVN, MOVK, MOVZ
			void put_inst_mov(Registry registry, uint16_t opc, uint16_t imm, uint16_t shift);

			/// Encode ORR instruction, using the given N:R:S fields
			void put_inst_orr_bitmask(Registry destination, Registry source, uint16_t n_immr_imms);

			/// Encode "ADC/ADCS (extended register)" operation
			void put_inst_adc(Registry destination, Registry a, Registry b, bool set_flags);

			/// Encode "SBC/SBCS" operation
			void put_inst_sbc(Registry destination, Registry a, Registry b, bool set_flags);

			/// Encode "BIC/BICS" operation
			void put_inst_bic(Registry dst, Registry a, Registry b, ShiftType shift, uint8_t lsl6, bool set_flags);

			/// Encode "CLS/CLZ" operation
			void put_inst_count(Registry destination, Registry source, uint8_t imm1);

			/// Encode "ILDR/LDRI/LDR" as well as the "ISTR/STRI/STR" operations
			void put_inst_ldst(Registry dst, Registry base, int64_t offset, Sizing sizing, MemoryOperation op, MemoryDirection dir);

			/// Encode "SMADDL/UMADDL" operation
			void put_inst_maddl(Registry dst, Registry a, Registry b, Registry addend, bool is_unsigned);

			/// Encode "UMULH/SMULH" operation
			void put_inst_mulh(Registry dst, Registry a, Registry b, bool is_unsigned);

			/// Encode "UDIV/UDIV" operation
			void put_inst_div(Registry dst, Registry a, Registry b, bool is_unsigned);

			/// Encode "REV16/REV32/REV64" operation
			void put_inst_rev(Registry dst, Registry src, uint16_t size_opc_10);

			/// Encode "LSL/LSR/ROR" operation
			void put_inst_shift_v(Registry dst, Registry src, Registry bits, ShiftType shift);

			/// Encode "CSINC/CSEL/CSET/CINC" operation
			void put_inst_csinc(Condition condition, Registry dst, Registry truthy, Registry falsy, bool increment_truth);

		public:

			void put_inst_add_imm(Registry destination, Registry source, uint16_t imm12, bool lsl_12 = false, bool set_flags = false);
			void put_inst_add_shifted(Registry destination, Registry a, Registry b, ShiftType shift, uint8_t imm6, bool set_flags = false);

		public:

			BufferWriter(SegmentedBuffer& buffer);

			// basic
			INST put_adc(Registry dst, Registry a, Registry b);            ///< Add with carry
			INST put_adcs(Registry dst, Registry a, Registry b);           ///< Add with carry and set flags
			INST put_add(Registry dst, Registry a, Registry b, Sizing size = Sizing::UX, uint8_t lsl3 = 0); ///< Add two registers, potentially extending one of them
			INST put_adds(Registry dst, Registry a, Registry b, Sizing size = Sizing::UX, uint8_t lsl3 = 0); ///< Add two registers, set the flags, potentially extending one of them
			INST put_adr(Registry destination, Label label);               ///< Form a PC-relative address
			INST put_adrp(Registry destination, Label label);              ///< Form a PC-page-relative address
			INST put_movz(Registry dst, uint16_t imm, uint16_t shift = 0); ///< Move shifted WORD into register, zero other bits
			INST put_movk(Registry dst, uint16_t imm, uint16_t shift = 0); ///< Move shifted WORD into register, keep other bits
			INST put_movn(Registry dst, uint16_t imm, uint16_t shift = 0); ///< Move shifted WORD into register, zero other bits, then NOT the register
			INST put_mov(Registry dst, uint64_t imm);                      ///< Move immediate into register
			INST put_mov(Registry dst, Registry src);                      ///< Move value between registers
			INST put_ret();                                                ///< Return from procedure using link register
			INST put_ret(Registry src);                                    ///< Return from procedure
			INST put_rbit(Registry dst, Registry src);                     ///< Reverse bits
			INST put_clz(Registry dst, Registry src);                      ///< Count leading zeros
			INST put_cls(Registry dst, Registry src);                      ///< Count leading signs (ones)
			INST put_ldr(Registry registry, Label label);                  ///< Load value from memory
			INST put_ildr(Registry dst, Registry base, int64_t offset, Sizing size); ///< Increment base and load value from memory
			INST put_ldri(Registry dst, Registry base, int64_t offset, Sizing size); ///< Load value from memory and increment base
			INST put_ldr(Registry registry, Registry base, uint64_t offset, Sizing size); ///< Load value from memory
			INST put_istr(Registry dst, Registry base, int64_t offset, Sizing size); ///< Increment base and store value to memory
			INST put_stri(Registry dst, Registry base, int64_t offset, Sizing size); ///< Store value to memory and increment base
			INST put_str(Registry registry, Registry base, uint64_t offset, Sizing size); ///< Store value to memory
			INST put_ands(Registry dst, Registry src, BitPattern pattern); ///< Bitwise AND between register and bit pattern
			INST put_and(Registry dst, Registry src, BitPattern pattern);  ///< Bitwise AND between register and bit pattern, set flags
			INST put_ands(Registry dst, Registry a, Registry b, ShiftType shift = ShiftType::LSL, uint8_t lsl6 = 0); ///< Bitwise AND between two register, shifting the second one, set flags
			INST put_and(Registry dst, Registry a, Registry b, ShiftType shift = ShiftType::LSL, uint8_t lsl6 = 0); ///< Bitwise AND between two register, shifting the second one
			INST put_eor(Registry destination, Registry source, BitPattern pattern); ///< Bitwise XOR between register and bit pattern
			INST put_eor(Registry dst, Registry a, Registry b, ShiftType shift = ShiftType::LSL, uint8_t lsl6 = 0); ///< Bitwise XOR between two register, shifting the second one
			INST put_orr(Registry destination, Registry source, BitPattern pattern); ///< Bitwise OR between register and bit pattern
			INST put_orr(Registry dst, Registry a, Registry b, ShiftType shift = ShiftType::LSL, uint8_t lsl6 = 0); ///< Bitwise OR between two register, shifting the second one
			INST put_sbc(Registry dst, Registry a, Registry b);            ///< Subtract with Carry
			INST put_sbcs(Registry dst, Registry a, Registry b);           ///< Subtract with Carry and set flags
			INST put_sub(Registry dst, Registry a, Registry b, Sizing size = Sizing::UX, uint8_t lsl3 = 0); ///< Add two registers, potentially extending one of them
			INST put_subs(Registry dst, Registry a, Registry b, Sizing size = Sizing::UX, uint8_t lsl3 = 0); ///< Add two registers, set the flags, potentially extending one of them
			INST put_cmp(Registry a, Registry b, Sizing size = Sizing::UX, uint8_t lsl3 = 0); ///< Compare
			INST put_cmn(Registry a, Registry b, Sizing size = Sizing::UX, uint8_t lsl3 = 0); ///< Compare negative
			INST put_madd(Registry dst, Registry a, Registry b, Registry addend); ///< Multiply and Add 64 bit registers
			INST put_smaddl(Registry dst, Registry a, Registry b, Registry addend); ///< Signed multiply two 32 bit registers and add 64 bit register
			INST put_umaddl(Registry dst, Registry a, Registry b, Registry addend); ///< Unsigned multiply two 32 bit registers and add 64 bit register
			INST put_mul(Registry dst, Registry a, Registry b);            ///< Multiply 64 bit registers
			INST put_smul(Registry dst, Registry a, Registry b);           ///< Signed multiply 32 bit registers
			INST put_umul(Registry dst, Registry a, Registry b);           ///< Unsigned multiply 32 bit registers
			INST put_smulh(Registry dst, Registry a, Registry b);          ///< Signed multiply high
			INST put_umulh(Registry dst, Registry a, Registry b);          ///< Unsigned multiply high
			INST put_sdiv(Registry dst, Registry a, Registry b);           ///< Signed Divide
			INST put_udiv(Registry dst, Registry a, Registry b);           ///< Unsigned Divide
			INST put_rev16(Registry dst, Registry src);                    ///< Reverse bytes in 16-bit words
			INST put_rev32(Registry dst, Registry src);                    ///< Reverse bytes in 32-bit dwords
			INST put_rev64(Registry dst, Registry src);                    ///< Reverse bytes in 64-bit qwords
			INST put_ror(Registry dst, Registry src, Registry bits);       ///< Rotate Right by register
			INST put_lsr(Registry dst, Registry src, Registry bits);       ///< Logical Shift Right by register
			INST put_lsl(Registry dst, Registry src, Registry bits);       ///< Logical Shift Left by register
			INST put_asr(Registry dst, Registry src, Registry bits);       ///< Arithmetic Shift Right by register
			INST put_asl(Registry dst, Registry src, Registry bits);       ///< Arithmetic Shift Left by register
			INST put_ror(Registry dst, Registry src, uint8_t imm);         ///< Rotate Right by immediate
			INST put_extr(Registry dst, Registry left, Registry right, uint8_t imm5); ///< Extract register
			INST put_csel(Condition condition, Registry dst, Registry truthy, Registry falsy); ///< Conditional Select
			INST put_csinc(Condition condition, Registry dst, Registry truthy, Registry falsy); ///< Conditional Select and Increment if false
			INST put_cinc(Condition condition, Registry dst, Registry src);///< Conditional Increment if true
			INST put_cinc(Condition condition, Registry dst);              ///< Conditional Increment if true
			INST put_cset(Condition condition, Registry dst);              ///< Conditional Set if true
			INST put_tst(Registry a, Registry b, ShiftType shift = ShiftType::LSL, uint8_t lsl6 = 0); ///< Test shifted register
			INST put_sbfm(Registry dst, Registry src, BitPattern pattern); ///< Signed Bitfield Insert in Zero
			INST put_ubfm(Registry dst, Registry src, BitPattern pattern); ///< Unsigned Bitfield Insert in Zero
			INST put_uxtb(Registry dst, Registry src);                     ///< Extract Byte
			INST put_uxth(Registry dst, Registry src);                     ///< Extract Two Bytes
			INST put_bfm(Registry dst, Registry src, BitPattern pattern);  ///< Bitfield Move
			INST put_bfc(Registry dst, BitPattern pattern);                ///< Bitfield Clear
			INST put_bic(Registry dst, Registry a, Registry b, ShiftType shift = ShiftType::LSL, uint8_t lsl6 = 0); ///< Bitwise Bit Clear
			INST put_bics(Registry dst, Registry a, Registry b, ShiftType shift = ShiftType::LSL, uint8_t lsl6 = 0); ///< Bitwise Bit Clear and set flags

			// control
			INST put_svc(uint16_t imm16);                                  ///< Supervisor call
			INST put_hvc(uint16_t imm16);                                  ///< Hypervisor Call
			INST put_smc(uint16_t imm16);                                  ///< Secure Monitor Call
			INST put_hlt(uint16_t imm16);                                  ///< Halt
			INST put_brk(uint16_t imm16);                                  ///< Breakpoint Instruction exception
			INST put_hint(uint8_t imm7);                                   ///< Architectural hint
			INST put_isb();                                                ///< Instruction Synchronization Barrier
			INST put_nop();                                                ///< No operation
			INST put_yield();                                              ///< Indicate spin-lock
			INST put_wfe();                                                ///< Wait For Event
			INST put_wfi();                                                ///< Wait For Interrupt
			INST put_sev();                                                ///< Send Event
			INST put_sevl();                                               ///< Send Event Local
			INST put_esb();                                                ///< Error Synchronization Barrier
			INST put_psb();                                                ///< Profiling Synchronization Barrier.

			// branch
			INST put_b(const Label& label);                                ///< Branch
			INST put_b(Condition condition, const Label& label);           ///< Branch conditionally
			INST put_bl(const Label& label);                               ///< Branch with link
			INST put_blr(Registry ptr);                                    ///< Branch with link to register
			INST put_br(Registry ptr);                                     ///< Branch to register
			INST put_cbnz(Registry src, const Label& label);               ///< Branch if register is not zero
			INST put_cbz(Registry src, const Label& label);                ///< Branch if register is zero
			INST put_tbz(Registry test, uint16_t bit6, const Label& label);  ///< Test bit and Branch if Zero
			INST put_tbnz(Registry test, uint16_t bit6, const Label& label); ///< Test bit and Branch if Not Zero

	};

}
