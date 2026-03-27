#pragma once

#include <external.hpp>
#include <macro.hpp>

namespace asmio {

	enum struct ElfRelocationType : uint32_t {

		NONE = 0, // No relocation.

		X86_64_64 = 1, // Direct 64 bit
		X86_64_PC32 = 2, // PC relative 32 bit signed
		X86_64_GOT32 = 3, // 32 bit GOT entry
		X86_64_PLT32 = 4, // 32 bit PLT address
		X86_64_COPY = 5, // Copy symbol at runtime
		X86_64_GLOB_DAT = 6, // Create GOT entry
		X86_64_JUMP_SLOT = 7, // Create PLT entry
		X86_64_RELATIVE = 8, // Adjust by program base
		X86_64_GOTPCREL = 9, // 32 bit signed PC relative offset to GOT
		X86_64_32 = 10, // Direct 32 bit zero extended
		X86_64_32S = 11, // Direct 32 bit sign extended
		X86_64_16 = 12, // Direct 16 bit zero extended
		X86_64_PC16 = 13, // 16 bit sign extended pc relative
		X86_64_8 = 14, // Direct 8 bit sign extended
		X86_64_PC8 = 15, // 8 bit sign extended pc relative
		X86_64_DTPMOD64 = 16, // ID of module containing symbol
		X86_64_DTPOFF64 = 17, // Offset in module's TLS block
		X86_64_TPOFF64 = 18, // Offset in initial TLS block
		X86_64_TLSGD = 19, // 32 bit signed PC relative offset to two GOT entries for GD symbol
		X86_64_TLSLD = 20, // 32 bit signed PC relative offset to two GOT entries for LD symbol
		X86_64_DTPOFF32 = 21, // Offset in TLS block
		X86_64_GOTTPOFF = 22, // 32 bit signed PC relative offset  to GOT entry for IE symbol
		X86_64_TPOFF32 = 23, // Offset in initial TLS block
		X86_64_PC64 = 24, // PC relative 64 bit
		X86_64_GOTOFF64 = 25, // 64 bit offset to GOT
		X86_64_GOTPC32 = 26, // 32 bit signed pc relative offset to GOT
		X86_64_GOT64 = 27, // 64-bit GOT entry offset
		X86_64_GOTPCREL64 = 28, // 64-bit PC relative offset to GOT entry
		X86_64_GOTPC64 = 29, // 64-bit PC relative offset to GOT
		X86_64_GOTPLT64 = 30, // like GOT64, says PLT entry needed
		X86_64_PLTOFF64 = 31, // 64-bit GOT relative offset  to PLT entry
		X86_64_SIZE32 = 32, // Size of symbol plus 32-bit addend
		X86_64_SIZE64 = 33, // Size of symbol plus 64-bit addend
		X86_64_GOTPC32_TLSDESC = 34, // GOT offset for TLS descriptor.
		X86_64_TLSDESC_CALL = 35, // Marker for call through TLS  descriptor.
		X86_64_TLSDESC = 36, // TLS descriptor.
		X86_64_IRELATIVE = 37, // Adjust indirectly by program base
		X86_64_RELATIVE64 = 38, // 64-bit adjust by program base
		X86_64_GOTPCRELX = 41, // Load from 32 bit signed pc relative offset to GOT entry without REX prefix, relaxable.
		X86_64_REX_GOTPCRELX = 42, // Load from 32 bit signed pc relative offset to GOT entry with REX prefix, relaxable.
				
		AARCH64_ABS64 = 257, // Direct 64 bit. 
		AARCH64_ABS32 = 258, // Direct 32 bit.
		AARCH64_ABS16 = 259, // Direct 16-bit.
		AARCH64_PREL64 = 260, // PC-relative 64-bit.	
		AARCH64_PREL32 = 261, // PC-relative 32-bit.	
		AARCH64_PREL16 = 262, // PC-relative 16-bit.	
		AARCH64_MOVW_UABS_G0 = 263, // Dir. MOVZ imm. from bits 15:0.
		AARCH64_MOVW_UABS_G0_NC = 264, // Likewise for MOVK; no check.
		AARCH64_MOVW_UABS_G1 = 265, // Dir. MOVZ imm. from bits 31:16.
		AARCH64_MOVW_UABS_G1_NC = 266, // Likewise for MOVK; no check.
		AARCH64_MOVW_UABS_G2 = 267, // Dir. MOVZ imm. from bits 47:32.
		AARCH64_MOVW_UABS_G2_NC = 268, // Likewise for MOVK; no check.
		AARCH64_MOVW_UABS_G3 = 269, // Dir. MOV{K,Z} imm. from 63:48.
		AARCH64_MOVW_SABS_G0 = 270, // Dir. MOV{N,Z} imm. from 15:0.
		AARCH64_MOVW_SABS_G1 = 271, // Dir. MOV{N,Z} imm. from 31:16.
		AARCH64_MOVW_SABS_G2 = 272, // Dir. MOV{N,Z} imm. from 47:32.
		AARCH64_LD_PREL_LO19v273, // PC-rel. LD imm. from bits 20:2.
		AARCH64_ADR_PREL_LO21 = 274, // PC-rel. ADR imm. from bits 20:0.
		AARCH64_ADR_PREL_PG_HI21 = 275, // Page-rel. ADRP imm. from 32:12.
		AARCH64_ADR_PREL_PG_HI21_NC = 276, // Likewise; no overflow check.
		AARCH64_ADD_ABS_LO12_NC = 277, // Dir. ADD imm. from bits 11:0.
		AARCH64_LDST8_ABS_LO12_NC = 278, // Likewise for LD/ST; no check. 
		AARCH64_TSTBR14 = 279, // PC-rel. TBZ/TBNZ imm. from 15:2.
		AARCH64_CONDBR19 = 280, // PC-rel. cond. br. imm. from 20:2. 
		AARCH64_JUMP26 = 282, // PC-rel. B imm. from bits 27:2.
		AARCH64_CALL26 = 283, // Likewise for CALL.
		AARCH64_LDST16_ABS_LO12_NC = 284, // Dir. ADD imm. from bits 11:1.
		AARCH64_LDST32_ABS_LO12_NC = 285, // Likewise for bits 11:2.
		AARCH64_LDST64_ABS_LO12_NC = 286, // Likewise for bits 11:3.
		AARCH64_MOVW_PREL_G0 = 287, // PC-rel. MOV{N,Z} imm. from 15:0.
		AARCH64_MOVW_PREL_G0_NC = 288, // Likewise for MOVK; no check.
		AARCH64_MOVW_PREL_G1 = 289, // PC-rel. MOV{N,Z} imm. from 31:16. 
		AARCH64_MOVW_PREL_G1_NC = 290, // Likewise for MOVK; no check.
		AARCH64_MOVW_PREL_G2 = 291, // PC-rel. MOV{N,Z} imm. from 47:32.
		AARCH64_MOVW_PREL_G2_NC = 292, // Likewise for MOVK; no check.
		AARCH64_MOVW_PREL_G3 = 293, // PC-rel. MOV{N,Z} imm. from 63:48.
		AARCH64_LDST128_ABS_LO12_NC = 299, // Dir. ADD imm. from bits 11:4.
		AARCH64_MOVW_GOTOFF_G0 = 300, // GOT-rel. off. MOV{N,Z} imm. 15:0.
		AARCH64_MOVW_GOTOFF_G0_NC = 301, // Likewise for MOVK; no check.
		AARCH64_MOVW_GOTOFF_G1 = 302, // GOT-rel. o. MOV{N,Z} imm. 31:16.
		AARCH64_MOVW_GOTOFF_G1_NC = 303, // Likewise for MOVK; no check.
		AARCH64_MOVW_GOTOFF_G2 = 304, // GOT-rel. o. MOV{N,Z} imm. 47:32.
		AARCH64_MOVW_GOTOFF_G2_NC = 305, // Likewise for MOVK; no check.
		AARCH64_MOVW_GOTOFF_G3 = 306, // GOT-rel. o. MOV{N,Z} imm. 63:48.
		AARCH64_GOTREL64 = 307, // GOT-relative 64-bit.
		AARCH64_GOTREL32 = 308, // GOT-relative 32-bit.
		AARCH64_GOT_LD_PREL19 = 309, // PC-rel. GOT off. load imm. 20:2.
		AARCH64_LD64_GOTOFF_LO15 = 310, // GOT-rel. off. LD/ST imm. 14:3.
		AARCH64_ADR_GOT_PAGE = 311, // P-page-rel. GOT off. ADRP 32:12.
		AARCH64_LD64_GOT_LO12_NC = 312, // Dir. GOT off. LD/ST imm. 11:3.
		AARCH64_LD64_GOTPAGE_LO15 = 313, // GOT-page-rel. GOT off. LD/ST 14:3
		AARCH64_TLSGD_ADR_PREL21 = 512, // PC-relative ADR imm. 20:0.
		AARCH64_TLSGD_ADR_PAGE21 = 513, // page-rel. ADRP imm. 32:12.
		AARCH64_TLSGD_ADD_LO12_NC = 514, // direct ADD imm. from 11:0.
		AARCH64_TLSGD_MOVW_G1 = 515, // GOT-rel. MOV{N,Z} 31:16.
		AARCH64_TLSGD_MOVW_G0_NC = 516, // GOT-rel. MOVK imm. 15:0.
		AARCH64_TLSLD_ADR_PREL21 = 517, // Like 512; local dynamic model.
		AARCH64_TLSLD_ADR_PAGE21 = 518, // Like 513; local dynamic model.
		AARCH64_TLSLD_ADD_LO12_NC = 519, // Like 514; local dynamic model.
		AARCH64_TLSLD_MOVW_G1 = 520, // Like 515; local dynamic model.
		AARCH64_TLSLD_MOVW_G0_NC = 521, // Like 516; local dynamic model.
		AARCH64_TLSLD_LD_PREL19 = 522, // TLS PC-rel. load imm. 20:2.
		AARCH64_TLSLD_MOVW_DTPREL_G2 = 523, // TLS DTP-rel. MOV{N,Z} 47:32.
		AARCH64_TLSLD_MOVW_DTPREL_G1 = 524, // TLS DTP-rel. MOV{N,Z} 31:16.
		AARCH64_TLSLD_MOVW_DTPREL_G1_NC = 525, // Likewise; MOVK; no check.
		AARCH64_TLSLD_MOVW_DTPREL_G0 = 526, // TLS DTP-rel. MOV{N,Z} 15:0.
		AARCH64_TLSLD_MOVW_DTPREL_G0_NC = 527, // Likewise; MOVK; no check.
		AARCH64_TLSLD_ADD_DTPREL_HI12 = 528, // DTP-rel. ADD imm. from 23:12.
		AARCH64_TLSLD_ADD_DTPREL_LO12 = 529, // DTP-rel. ADD imm. from 11:0.
		AARCH64_TLSLD_ADD_DTPREL_LO12_NC = 530, // Likewise; no ovfl. check.
		AARCH64_TLSLD_LDST8_DTPREL_LO12 = 531, // DTP-rel. LD/ST imm. 11:0.
		AARCH64_TLSLD_LDST8_DTPREL_LO12_NC = 532, // Likewise; no check.
		AARCH64_TLSLD_LDST16_DTPREL_LO12 = 533, // DTP-rel. LD/ST imm. 11:1.
		AARCH64_TLSLD_LDST16_DTPREL_LO12_NC = 534, // Likewise; no check.
		AARCH64_TLSLD_LDST32_DTPREL_LO12 = 535, // DTP-rel. LD/ST imm. 11:2.
		AARCH64_TLSLD_LDST32_DTPREL_LO12_NC = 536, // Likewise; no check.
		AARCH64_TLSLD_LDST64_DTPREL_LO12 = 537, // DTP-rel. LD/ST imm. 11:3.
		AARCH64_TLSLD_LDST64_DTPREL_LO12_NC = 538, // Likewise; no check.
		AARCH64_TLSIE_MOVW_GOTTPREL_G1 = 539, // GOT-rel. MOV{N,Z} 31:16.
		AARCH64_TLSIE_MOVW_GOTTPREL_G0_NC = 540, // GOT-rel. MOVK 15:0.
		AARCH64_TLSIE_ADR_GOTTPREL_PAGE21 = 541, // Page-rel. ADRP 32:12.
		AARCH64_TLSIE_LD64_GOTTPREL_LO12_NC = 542, // Direct LD off. 11:3.
		AARCH64_TLSIE_LD_GOTTPREL_PREL19 = 543, // PC-rel. load imm. 20:2.
		AARCH64_TLSLE_MOVW_TPREL_G2 = 544, // TLS TP-rel. MOV{N,Z} 47:32.
		AARCH64_TLSLE_MOVW_TPREL_G1 = 545, // TLS TP-rel. MOV{N,Z} 31:16.
		AARCH64_TLSLE_MOVW_TPREL_G1_NC = 546, // Likewise; MOVK; no check.
		AARCH64_TLSLE_MOVW_TPREL_G0 = 547, // TLS TP-rel. MOV{N,Z} 15:0.
		AARCH64_TLSLE_MOVW_TPREL_G0_NC = 548, // Likewise; MOVK; no check.
		AARCH64_TLSLE_ADD_TPREL_HI12 = 549, // TP-rel. ADD imm. 23:12.
		AARCH64_TLSLE_ADD_TPREL_LO12 = 550, // TP-rel. ADD imm. 11:0.
		AARCH64_TLSLE_ADD_TPREL_LO12_NC = 551, // Likewise; no ovfl. check.
		AARCH64_TLSLE_LDST8_TPREL_LO12 = 552, // TP-rel. LD/ST off. 11:0.
		AARCH64_TLSLE_LDST8_TPREL_LO12_NC = 553, // Likewise; no ovfl. check.
		AARCH64_TLSLE_LDST16_TPREL_LO12 = 554, // TP-rel. LD/ST off. 11:1.
		AARCH64_TLSLE_LDST16_TPREL_LO12_NC = 555, // Likewise; no check.
		AARCH64_TLSLE_LDST32_TPREL_LO12 = 556, // TP-rel. LD/ST off. 11:2.
		AARCH64_TLSLE_LDST32_TPREL_LO12_NC = 557, // Likewise; no check.
		AARCH64_TLSLE_LDST64_TPREL_LO12 = 558, // TP-rel. LD/ST off. 11:3.
		AARCH64_TLSLE_LDST64_TPREL_LO12_NC = 559, // Likewise; no check.
		AARCH64_TLSDESC_LD_PREL19 = 560, // PC-rel. load immediate 20:2.
		AARCH64_TLSDESC_ADR_PREL21 = 561, // PC-rel. ADR immediate 20:0.
		AARCH64_TLSDESC_ADR_PAGE21 = 562, // Page-rel. ADRP imm. 32:12.
		AARCH64_TLSDESC_LD64_LO12 = 563, // Direct LD off. from 11:3.
		AARCH64_TLSDESC_ADD_LO12 = 564, // Direct ADD imm. from 11:0.
		AARCH64_TLSDESC_OFF_G1 = 565, // GOT-rel. MOV{N,Z} imm. 31:16.
		AARCH64_TLSDESC_OFF_G0_NC = 566, // GOT-rel. MOVK imm. 15:0; no ck.
		AARCH64_TLSDESC_LDR = 567, // Relax LDR.
		AARCH64_TLSDESC_ADD = 568, // Relax ADD.
		AARCH64_TLSDESC_CALL = 569, // Relax BLR.
		AARCH64_TLSLE_LDST128_TPREL_LO12 = 570, // TP-rel. LD/ST off. 11:4.
		AARCH64_TLSLE_LDST128_TPREL_LO12_NC = 571, // Likewise; no check.
		AARCH64_TLSLD_LDST128_DTPREL_LO12 = 572, // DTP-rel. LD/ST imm. 11:4.
		AARCH64_TLSLD_LDST128_DTPREL_LOf12_NC = 573, // Likewise; no check.
		AARCH64_COPY = 1024, // Copy symbol at runtime.
		AARCH64_GLOB_DAT = 1025, // Create GOT entry.
		AARCH64_JUMP_SLOT = 1026, // Create PLT entry.
		AARCH64_RELATIVE = 1027, // Adjust by program base.
		AARCH64_TLS_DTPMOD = 1028, // Module number, 64 bit.
		AARCH64_TLS_DTPREL = 1029, // Module-relative offset, 64 bit.
		AARCH64_TLS_TPREL = 1030, // TP-relative offset, 64 bit.
		AARCH64_TLSDESC = 1031, // TLS Descriptor.
		AARCH64_IRELATIVE = 1032, // STT_GNU_IFUNC relocation.
		
		RISCV_32 = 1,
		RISCV_64 = 2,
		RISCV_RELATIVE = 3,
		RISCV_COPY = 4,
		RISCV_JUMP_SLOT = 5,
		RISCV_TLS_DTPMOD32 = 6,
		RISCV_TLS_DTPMOD64 = 7,
		RISCV_TLS_DTPREL32 = 8,
		RISCV_TLS_DTPREL64 = 9,
		RISCV_TLS_TPREL32 = 10,
		RISCV_TLS_TPREL64 = 11,
		RISCV_TLSDESC = 12,
		RISCV_BRANCH = 16,
		RISCV_JAL = 17,
		RISCV_CALL = 18,
		RISCV_CALL_PLT = 19,
		RISCV_GOT_HI20 = 20,
		RISCV_TLS_GOT_HI20 = 21,
		RISCV_TLS_GD_HI20 = 22,
		RISCV_PCREL_HI20 = 23,
		RISCV_PCREL_LO12_I = 24,
		RISCV_PCREL_LO12_S = 25,
		RISCV_HI20 = 26,
		RISCV_LO12_I = 27,
		RISCV_LO12_S = 28,
		RISCV_TPREL_HI20 = 29,
		RISCV_TPREL_LO12_I = 30,
		RISCV_TPREL_LO12_S = 31,
		RISCV_TPREL_ADD = 32,
		RISCV_ADD8 = 33,
		RISCV_ADD16 = 34,
		RISCV_ADD32 = 35,
		RISCV_ADD64 = 36,
		RISCV_SUB8 = 37,
		RISCV_SUB16 = 38,
		RISCV_SUB32 = 39,
		RISCV_SUB64 = 40,
		RISCV_GOT32_PCREL = 41,
		RISCV_ALIGN = 43,
		RISCV_RVC_BRANCH = 44,
		RISCV_RVC_JUMP = 45,
		RISCV_RELAX = 51,
		RISCV_SUB6 = 52,
		RISCV_SET6 = 53,
		RISCV_SET8 = 54,
		RISCV_SET16 = 55,
		RISCV_SET32 = 56,
		RISCV_32_PCREL = 57,
		RISCV_IRELATIVE = 58,
		RISCV_PLT32 = 59,
		RISCV_SET_ULEB128 = 60,
		RISCV_SUB_ULEB128 = 61,
		RISCV_TLSDESC_HI20 = 62,
		RISCV_TLSDESC_LOAD_LO12 = 63,
		RISCV_TLSDESC_ADD_LO12 = 64,
		RISCV_TLSDESC_CALL = 65,
		
	};

	struct PACKED ElfRelocationInfo {
		uint32_t sym;
		ElfRelocationType type;
	};

	struct PACKED ElfImplicitRelocation {
		uint64_t offset;
		ElfRelocationInfo info;
	};

	struct PACKED ElfExplicitRelocation {
		uint64_t offset;
		ElfRelocationInfo info;
		int64_t addend;
	};

}