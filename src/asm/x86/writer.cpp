#include "writer.hpp"

// private libs
#include <iostream>

namespace asmio::x86 {

	void BufferWriter::put_inst_rex(bool w, bool r, bool x, bool b) {

		//   7 6 5 4   3   2   1   0
		// + ------- + - + - + - + - +
		// | 0 1 0 0 | W | R | X | B |
		// + ------- + - + - + - + - +
		//   |       |   |   |   |
		//  Fixed    |   |   |   \_ bit 4 of MODRM.rm SIB.base
		//  Pattern  |   |   \_ bit 4 of SIB.index
		//           |   \_ bit 4 of MODRM.reg
		//           \_ 64 bit operand prefix

		// REX prefix with no flags set still has an effect on the encoding, when present
		// High Byte Registers (AH, DH, ...) become unaccessible in favor of the new Low Byte Registers (SIL, DIL, ...)

		put_byte(0b0100'0000 | (w ? 0b1000 : 0) | (r ? 0b0100 : 0) | (x ? 0b0010 : 0) | (b ? 0b0001 : 0));

	}

	uint8_t BufferWriter::pack_opcode_dw(uint8_t opcode, bool d, bool w) {

		//   7 6 5 4 3 2   1   0
		// + ----------- + - + - +
		// | opcode      | d | w |
		// + ----------- + - + - +
		// |             |   |
		// |             |   \_ wide flag
		// |             \_ direction flag
		// \_ operation code

		// Wide flag controls which set of registers is encoded
		// in the subsequent bytes

		// Direction flag controls the operation direction
		// with d=1 being reg <= r/m, and d=0 reg => r/m

		// Some operations expect direction to be set to a specific value,
		// so it is sometimes considered to be a part of the opcode itself

		return (opcode << 2 | (d << 1) | w);
	}

	void BufferWriter::put_inst_mod_reg_rm(uint8_t mod, uint8_t reg, uint8_t r_m) {

		//   7 6   5 4 3   2 1 0
		// + --- + ----- + ----- +
		// | mod | reg   | r/m   |
		// + --- + ----- + ----- +
		// |     |       |
		// |     |       \_ encodes register
		// |     \_ encodes register or instruction specific data
		// \_ offset size

		// If mod is set to MOD_SHORT (0b11) then both reg and r/m are treated
		// as simple register references, for example in "mov eax, edx"

		// If mod is set to any other value (MOD_NONE, MOD_BYTE, MOD_QUAD)
		// then the r/m is treated as a pointer, for example in "mov [eax], edx"

		// Mod also controls the length of the offset in bytes, with MOD_NONE having no offset,
		// MOD_BYTE having a single byte of offset after the instruction, and MOD_QUAD having four.

		// if mod is NOT equal MOD_SHORT, and r/m is set to RM_SIB (0b100, same value as ESP)
		// then the SIB byte is expected after this byte

		put_byte(r_m | (reg << 3) | (mod << 6));
	}

	void BufferWriter::put_inst_sib(uint8_t ss, uint8_t index, uint8_t base) {

		//   7 6   5 4 3   2 1 0
		// + --- + ----- + ----- +
		// | ss  | index | base  |
		// + --- + ----- + ----- +
		// |     |       |
		// |     |       \_ base registry
		// |     \_ index registry
		// \_ scaling factor

		// Index register can be omitted from SIB by setting 'index' to NO_SIB_INDEX (0b100) and
		// 'ss' to NO_SIB_SCALE (0b00), this is useful for encoding the ESP registry

		put_byte(base | (index << 3) | (ss << 6));
	}

	void BufferWriter::put_inst_imm(uint64_t immediate, uint8_t width) {
		buffer.insert((uint8_t *) &immediate, std::min(width, QWORD));
	}

	void BufferWriter::put_linker_command(const Label& label, int32_t addend, int32_t shift, uint8_t width, LinkType type) {

		// cap at maximum supported size
		if (width > QWORD) {
			width = QWORD;
		}

		buffer.add_linkage(label, shift, [width, type, addend] (SegmentedBuffer* buffer, const Linkage& linkage, size_t mount) {
			LabelMarker src = buffer->get_label(linkage.label);
			LabelMarker dst = linkage.target;

			const int64_t offset = (type == RELATIVE)
				? buffer->get_offset(dst)
				: -mount;

			const int64_t value = buffer->get_offset(src) - offset + addend;
			const uint8_t* value_ptr = reinterpret_cast<const uint8_t*>(&value);
			memcpy(buffer->get_pointer(dst), value_ptr, width);
		});
	}

	void BufferWriter::put_inst_label_imm(Location imm, uint8_t width) {

		// cap at maximum supported size
		if (width > QWORD) {
			width = QWORD;
		}

		if (imm.is_labeled()) {
			put_linker_command(imm.label, imm.offset, 0, width, ABSOLUTE);
		}

		put_inst_imm(imm.offset, width);
	}

	void BufferWriter::put_inst_std(uint8_t opcode, Location dst, RegInfo packed, uint8_t size, bool longer) {

		// always query suffix size to clear it when not used
		const int suffix_bytes = get_suffix();

		if (size == VOID) {
			throw std::runtime_error {"Unable to deduce operand size"};
		}

		// this assumes that both operands have the same size
		if (size == WORD) {
			put_16bit_operand_prefix();
		}

		// all this is needed to prepend the address size prefix
		if (dst.is_memory()) {

			uint8_t adr_size = VOID;

			if (dst.base != UNSET) {
				adr_size = dst.base.size;
			}

			if (dst.index != UNSET) {

				// check if the size is correct
				// we can use [eax + edx] and [rax + rdx] but not [eax + rdx]
				if (adr_size != VOID && adr_size != dst.index.size) {
					throw std::runtime_error {"Inconsistent address size used"};
				}

				adr_size = dst.index.size;
			}

			// switch to 32 bit addressing
			if (adr_size == DWORD) {
				put_32bit_address_prefix();
			}

			// in long mode only 32 and 64 bit addresses are valid
			// VOID address size means there was no base/index
			else if (adr_size != VOID && adr_size != QWORD) {
				throw std::runtime_error {"Invalid address size"};
			}

		}

		// simple registry to registry operation
		if (dst.is_simple()) {

			if (packed.rex || dst.base.is(Registry::REX) || size == QWORD) {
				put_inst_rex(size == QWORD, packed.is_extended(), false, dst.base.reg & 0b1000);
			}

			// two byte opcode, starts with 0x0F
			if (longer) {
				put_byte(LONG_OPCODE);
			}

			put_byte(opcode);
			put_inst_mod_reg_rm(MOD_SHORT, packed.reg, dst.base.low());
			return;
		}

		// this is where the fun begins ...
		uint8_t sib_scale = dst.get_ss_flag();
		uint8_t sib_index = dst.index.reg;
		uint8_t sib_base = dst.base.reg;
		uint8_t mrm_mod = dst.get_mod_flag();
		uint8_t mrm_mem = dst.base.reg;
		uint8_t imm_len = DWORD;
		bool rip_relative = false;

		// in most cases mod controls the size of offset (immediate value)
		// but there are exceptions that cause those two to be different
		if (mrm_mod == MOD_NONE) imm_len = VOID;
		if (mrm_mod == MOD_BYTE) imm_len = BYTE;

		// if there is no base or index (just the offset), put NO_BASE into r/m, and MOD_NONE into mod
		// this is a special case used to encode a direct offset reference (32 bit)
		if (dst.base == UNSET && dst.index == UNSET) {
			mrm_mod = MOD_NONE;

			if (dst.is_labeled()) {

				// this encodes a RIP + offset in long mode
				mrm_mem = NO_BASE;
				rip_relative = true;

			} else {

				// for a direct virtual address (like it used to work in 32 bit)
				// we need to use SIB with base=none, index=none
				mrm_mem = RM_SIB;
				sib_base = NO_BASE;
				sib_index = NO_SIB_INDEX;
				sib_scale = NO_SIB_SCALE;

			}

			// 32 bits unless REX.W prefix is present
			imm_len = DWORD;
		}

		// special case for [EBP/RBP/R13]
		// we have to encode it as [EBP/RBP/R13 + 0]
		else if (dst.base.is_ebp_like() && mrm_mod == MOD_NONE && dst.index == UNSET) {
			mrm_mod = MOD_BYTE;
			imm_len = BYTE;
		}

		// we have to use the SIB byte to target ESP/RSP
		else if (dst.base.is_esp_like() || dst.is_indexed()) {
			mrm_mem = RM_SIB;

			// special case for [EBP/RBP/R13 + (indexed)]
			// we have to encode it as [EBP/RBP/R13 + (indexed) + 0]
			if (dst.base.is_ebp_like() && mrm_mod == MOD_NONE) {
				mrm_mod = MOD_BYTE;
				imm_len = BYTE;
			}

			// no base in SIB
			// this is encoded by setting base to NO_BASE (101b) and mod to MOD_NONE (00b)
			if (dst.base == UNSET) {
				sib_base = NO_BASE;
				mrm_mod = MOD_NONE;

				// in this special case the size of offset is assumed to be 32 bits
				imm_len = DWORD;
			}

			// no index in SIB
			// this is encoded by setting index to NO_INDEX (100b) and ss to NO_SCALE (00b)
			if (dst.index == UNSET) {
				sib_index = NO_SIB_INDEX;
				sib_scale = NO_SIB_SCALE;
			}
		}

		// REX
		if (size == QWORD || packed.rex || (sib_index & REG_HIGH) || (sib_base & REG_HIGH)) {
			put_inst_rex(size == QWORD, packed.is_extended(), sib_index & REG_HIGH, (mrm_mem | sib_base) & REG_HIGH);
		}

		// two byte opcode, starts with 0x0F
		if (longer) {
			put_byte(LONG_OPCODE);
		}

		put_byte(opcode);
		put_inst_mod_reg_rm(mrm_mod, packed.low(), mrm_mem & REG_LOW);

		// if SIB was enabled write it
		if (mrm_mem == RM_SIB) {
			put_inst_sib(sib_scale, sib_index & REG_LOW, sib_base & REG_LOW);
		}

		// if offset was present write it
		if (imm_len != VOID) {

			// if a displacement only reference was used we can encode it as RIP-relative
			if (rip_relative) {
				put_label(dst.label, imm_len, dst.offset - suffix_bytes);
				return;
			}

			// otherwise just put the immediate as-is
			put_inst_label_imm(dst, imm_len);
		}

	}

	void BufferWriter::put_inst_std_ri(uint8_t opcode, Location dst, uint8_t inst) {
		put_inst_std_as(opcode, dst, RegInfo::raw(inst));
	}

	void BufferWriter::put_inst_std_as(uint8_t opcode, Location dst, RegInfo packed, bool longer) {
		put_inst_std(opcode, dst, packed, dst.size, longer);
	}

	void BufferWriter::put_inst_std_dw(uint8_t opcode, Location dst, RegInfo packed, uint8_t size, bool direction, bool wide, bool longer) {
		put_inst_std(pack_opcode_dw(opcode, direction, wide), dst, packed, size, longer);
	}

	void BufferWriter::put_inst_std_ds(uint8_t opcode, Location dst, RegInfo packed, uint8_t size, bool direction, bool longer) {
		put_inst_std_dw(opcode, dst, packed, size, direction, size != BYTE, longer);
	}

	void BufferWriter::put_inst_fpu(uint8_t opcode, uint8_t base, uint8_t sti) {
		put_byte(opcode);
		put_byte(base + sti);
	}

	/**
	 * Used for constructing the MOV instruction
	 */
	void BufferWriter::put_inst_mov(Location dst, Location src, bool direction) {

		// for immediate values this will equal 0
		uint8_t opr_size = pair_size(dst, src);

		if (src.is_immediate()) {
			set_suffix(opr_size);
		}

		put_inst_std_ds(src.is_immediate() ? 0b110001 : 0b100010, dst, src.base.pack(), opr_size, direction);

		if (src.is_immediate()) {
			put_inst_label_imm(src, opr_size);
		}
	}

	/**
	 * Used for constructing the MOVSX and MOVZX instructions
	 */
	void BufferWriter::put_inst_movx(uint8_t opcode, Location dst, Location src) {
		uint8_t dst_len = dst.size;
		uint8_t src_len = src.size;

		// 66h movsx (w=0) -> movsx [word], [byte]
		// 66h movsx (w=1) ->
		//     movsx (w=0) -> movsx [quad], [byte]
		//     movsx (w=1) -> movsx [quad], [word]

		if (!dst.is_simple()) {
			throw std::runtime_error {"Invalid destination operand"};
		}

		if (src_len >= dst_len) {
			throw std::runtime_error {"Invalid destination size"};
		}

		put_inst_std(pack_opcode_dw(opcode, true, src_len == WORD), src, dst.base.pack(), dst.size, true);
	}

	/**
	 * Used to for constructing the shift instructions
	 */
	void BufferWriter::put_inst_shift(Location dst, Location src, uint8_t inst) {

		RegInfo reg_opcode = RegInfo::raw(inst);

		if (src.is_simple() && src.base == CL) {
			put_inst_std_ds(0b110100, dst, reg_opcode, dst.size, true);

			return;
		}

		if (src.is_immediate()) {
			uint8_t src_val = src.offset;

			if (src_val == 1) {
				put_inst_std_ds(0b110100, dst, reg_opcode, pair_size(src, dst), false);
			} else {
				put_inst_std_ds(0b110000, dst, reg_opcode, pair_size(src, dst), false);
				put_byte(src_val);
			}

			return;
		}

		throw std::runtime_error {"Invalid operands"};

	}

	/**
	 * Used to for constructing the double shift instructions
	 */
	void BufferWriter::put_inst_double_shift(uint8_t opcode, Location dst, Location src, Location cnt) {

		if (cnt.is_immediate()) {
			set_suffix(1);
			put_inst_std(opcode | 0, dst, src.base.pack(), pair_size(src, dst), true);
			put_byte(cnt.offset);
			return;
		}

		if (cnt.is_simple() && cnt.base == CL) {
			put_inst_std(opcode | 1, dst, src.base.pack(), pair_size(src, dst), true);
			return;
		}

		throw std::runtime_error {"Invalid operands"};

	}

	void BufferWriter::put_inst_tuple(Location dst, Location src, uint8_t opcode_rmr, uint8_t opcode_reg) {

		const uint8_t opr_size = pair_size(src, dst);

		if (dst.is_simple() && src.is_memreg()) {
			put_inst_std_ds(opcode_rmr, src, dst.base.pack(), opr_size, true);
			return;
		}

		if (src.is_simple() && dst.reference) {
			put_inst_std_ds(opcode_rmr, dst, src.base.pack(), opr_size, false);
			return;
		}

		if (dst.is_memreg() && src.is_immediate()) {

			// all tuple instruction use a 32-bit capped immediate values
			uint8_t imm_size = std::min(DWORD, opr_size);

			set_suffix(imm_size);
			put_inst_std_ds(0b100000, dst, RegInfo::raw(opcode_reg), opr_size, false /* TODO: sign flag */);
			put_inst_label_imm(src, imm_size);
			return;
		}

		throw std::runtime_error {"Invalid operands"};

	}

	/**
	 * Used for constructing the Bit Test family of instructions
	 */
	void BufferWriter::put_inst_btx(Location dst, Location src, uint8_t opcode, uint8_t inst) {

		uint8_t opr_size = pair_size(dst, src);

		if (opr_size == BYTE) {
			throw std::runtime_error {"Invalid operand, byte register can't be used here"};
		}

		if (dst.is_memreg() && src.is_simple()) {
			put_inst_std_dw(opcode, dst, src.base.pack(), opr_size, true, true, true);
			return;
		}

		if (dst.is_memreg() && src.is_immediate()) {
			set_suffix(1);
			put_inst_std(0b10111010, dst, RegInfo::raw(inst), opr_size, true);
			put_byte(src.offset);
			return;
		}

		throw std::runtime_error {"Invalid operands"};

	}

	/**
	 * Used for constructing the 'conditional jump' family of instructions
	 */
	void BufferWriter::put_inst_jx(Location dst, uint8_t sopcode, uint8_t lopcode) {

		Label label = dst.label;
		long addend = dst.offset;

		if (!dst.is_jump_label()) {
			throw std::runtime_error {"Invalid operand"};
		}

		// FIXME
		// if (has_label(label)) {
		// 	long offset = get_label(label) - buffer.size() + addend;
		//
		// 	if (offset > -127) {
		// 		put_byte(sopcode);
		// 		put_label(label, BYTE, addend);
		// 		return;
		// 	}
		// }

		put_byte(0b00001111);
		put_byte(lopcode);
		put_label(label, DWORD, addend);

	}

	/**
	 * Used for constructing the 'set byte' family of instructions
	 */
	void BufferWriter::put_inst_setx(Location dst, uint8_t lopcode) {
		put_inst_std_as(0b1001'0000 | lopcode, dst, RegInfo::raw(0), true);
	}

	void BufferWriter::put_inst_rex(uint8_t wrxb) {
		put_byte(REX_PREFIX | wrxb);
	}

	void BufferWriter::put_rex_w() {
		put_byte(REX_PREFIX | REX_BIT_W);
	}

	void BufferWriter::put_16bit_operand_prefix() {
		put_byte(0b01100110);
	}

	void BufferWriter::put_32bit_address_prefix() {
		put_byte(0b01100111);
	}

	void BufferWriter::put_label(const Label& label, uint8_t size, int64_t addend) {
		put_linker_command(label, addend - size, 0, size, RELATIVE);

		while (size --> 0) {
			put_byte(0);
		}
	}

	bool BufferWriter::has_label(const Label& label) {
		return buffer.has_label(label);
	}

	int64_t BufferWriter::get_label(const Label& label) {
		return buffer.get_offset(buffer.get_label(label));
	}

	void BufferWriter::set_suffix(int suffix) {
		this->suffix = suffix;
	}

	int BufferWriter::get_suffix() {
		int suffix = this->suffix;
		this->suffix = 0;
		return suffix;
	}

	BufferWriter& BufferWriter::label(const Label& label) {
		buffer.add_label(label);
		return *this;
	}

	void BufferWriter::put_byte(uint8_t byte) {
		buffer.push(byte);
	}

	void BufferWriter::put_byte(std::initializer_list<uint8_t> bytes) {
		buffer.insert((uint8_t*) std::data(bytes), BYTE * bytes.size());
	}

	void BufferWriter::put_ascii(const std::string& str) {
		buffer.insert((uint8_t*) str.c_str(), BYTE * (str.size() + 1));
	}

	void BufferWriter::put_word(uint16_t word) {
		buffer.insert((uint8_t*) &word, WORD);
	}

	void BufferWriter::put_word(std::initializer_list<uint16_t> words) {
		buffer.insert((uint8_t*) std::data(words), WORD * words.size());
	}

	void BufferWriter::put_dword(uint32_t dword) {
		buffer.insert((uint8_t*) &dword, DWORD);
	}

	void BufferWriter::put_dword(std::initializer_list<uint32_t> dwords) {
		buffer.insert((uint8_t*) std::data(dwords), DWORD * dwords.size());
	}

	void BufferWriter::put_dword_f(float dword) {
		buffer.insert((uint8_t*) &dword, DWORD);
	}

	void BufferWriter::put_dword_f(std::initializer_list<float> dwords) {
		buffer.insert((uint8_t*) std::data(dwords), DWORD * dwords.size());
	}

	void BufferWriter::put_qword(uint64_t qword) {
		buffer.insert((uint8_t*) &qword, QWORD);
	}

	void BufferWriter::put_qword(std::initializer_list<uint64_t> dwords) {
		buffer.insert((uint8_t*) std::data(dwords), QWORD * dwords.size());
	}

	void BufferWriter::put_qword_f(double qword) {
		buffer.insert((uint8_t*) &qword, QWORD);
	}

	void BufferWriter::put_qword_f(std::initializer_list<double> dwords) {
		buffer.insert((uint8_t*) std::data(dwords), QWORD * dwords.size());
	}

	void BufferWriter::put_data(size_t bytes, void* date) {
		buffer.insert((uint8_t*) date, bytes);
	}

	void BufferWriter::put_space(size_t bytes, uint8_t value) {
		buffer.fill(bytes, value);
	}

	void BufferWriter::dump(bool verbose) const {
		// int i = 0;
		//
		// if (verbose) {
		// 	for (uint8_t byte : buffer) {
		// 		std::bitset<8> bin(byte);
		// 		std::cout << std::setfill('0') << std::setw(4) << i << " | ";
		// 		std::cout << std::setfill('0') << std::setw(2) << std::hex << ((int) byte) << ' ' << bin;
		// 		std::cout << " | " << std::dec << (char) (byte < ' ' || byte > '~' ? '.' : byte) << std::endl;
		// 		i++;
		// 	}
		// }
		//
		// std::cout << "./unasm.sh \"\" \"db ";
		// bool first = true;
		//
		// for (uint8_t byte : buffer) {
		// 	if (!first) {
		// 		std::cout << ", ";
		// 	}
		//
		// 	first = false;
		// 	std::cout << '0' << std::setfill('0') << std::setw(2) << std::hex << ((int) byte) << "h";
		// }
		//
		// std::cout << '"' << std::endl;
	}

	ExecutableBuffer BufferWriter::bake(bool debug) {
		return to_executable(buffer);
	}

	ElfBuffer BufferWriter::bake_elf(tasml::ErrorHandler* reporter, uint32_t address, const char* entry, bool debug) {
		// if (!has_label(entry)) {
		// 	throw std::runtime_error {"No entrypoint '" + std::string {entry} + "' defined!"};
		// }
		//
		// ElfBuffer elf {buffer.size(), address, (uint32_t) labels.at(entry)};
		// assemble(address + elf.offset(), reporter, debug);
		// elf.bake(buffer);
		//
		// return elf;
		throw std::runtime_error {"Not implemented yet"};
	}

}