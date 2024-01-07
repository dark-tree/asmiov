#include "writer.hpp"

// private libs
#include <iostream>

namespace asmio::x86 {

	uint8_t BufferWriter::pack_opcode_dw(uint8_t opcode, bool d, bool w) {

		//   7 6 5 4 3 2   1   0
		// + ----------- + - + - +
		// | opcode      | d | w |
		// + ----------- + - + - +
		// |             |   |
		// |             |   \_ wide flag
		// |             \_ direction flag
		// \_ operation code

		// Wide flag controls which set of register is encoded
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
		// |     \_ encodes register of instruction specific data
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

	void BufferWriter::put_inst_imm(uint32_t immediate, uint8_t width) {
		insert_buffer(buffer, (uint8_t *) &immediate, std::min(width, DWORD)); // limit to DWORD
	}

	void BufferWriter::put_inst_sib(Location ref) {
		uint8_t ss_flag = ref.get_ss_flag();
		uint8_t index_reg = ref.index.reg;

		if (ref.index.is(UNSET)) {
			index_reg = NO_SIB_INDEX;
			ss_flag = NO_SIB_SCALE;
		}

		put_inst_sib(ss_flag, index_reg, ref.base.reg);
	}

	void BufferWriter::put_inst_label_imm(Location imm, uint8_t size) {
		if (imm.is_labeled()) {
			commands.emplace_back(imm.label, size, buffer.size(), imm.offset, false);
		}

		put_inst_imm(imm.offset, size);
	}

	void BufferWriter::put_inst_std(uint8_t opcode, Location dst, uint8_t reg, bool longer) {
		uint8_t dst_reg = dst.base.reg;

		if (dst.size == WORD) {
			put_inst_16bit_operand_mark();
		}

		// two byte opcode, starts with 0x0F
		if (longer) {
			put_byte(0x0F);
		}

		if (dst.is_simple()) {
			put_byte(opcode);
			put_inst_mod_reg_rm(MOD_SHORT, reg, dst_reg);
			return;
		}

		// we have to use the SIB byte to target ESP
		bool sib = dst.base.is(ESP) || dst.is_indexed();
		uint8_t mod = dst.get_mod_flag();

		// special case for [EBP + (any indexed) + 0]
		if (dst.base.is(EBP)) {
			mod = MOD_BYTE;
		}

		// TODO: cleanup
		if (dst.base.is(UNSET)) {
			dst_reg = NO_BASE;
		}

		put_byte(opcode);
		put_inst_mod_reg_rm(dst_reg == NO_BASE ? MOD_NONE : mod, reg, sib ? RM_SIB : dst_reg);

		if (sib) {
			put_inst_sib(dst);
		}

		if (mod == MOD_BYTE || mod == MOD_QUAD) {
			put_inst_label_imm(dst, mod == MOD_BYTE ? BYTE : DWORD);
		}
	}

	void BufferWriter::put_inst_std(uint8_t opcode, Location dst, uint8_t reg, bool direction, bool wide, bool longer) {
		put_inst_std(pack_opcode_dw(opcode, direction, wide), dst, reg, longer);
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
		uint8_t src_reg = src.base.reg;
		uint8_t dst_len = dst.size;

		put_inst_std(src.is_immediate() ? 0b110001 : 0b100010, dst, src_reg, direction, dst.is_wide());

		if (src.is_immediate()) {
			put_inst_label_imm(src, dst_len);
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

		put_inst_std(opcode, src, dst.base.reg, true, src_len == WORD, true);
	}

	/**
	 * Used to for constructing the shift instructions
	 */
	void BufferWriter::put_inst_shift(Location dst, Location src, uint8_t inst) {

		bool dst_wide = dst.is_wide();

		if (src.is_simple() && src.base.is(CL)) {
			put_inst_std(0b110100, dst, inst, true, dst_wide);

			return;
		}

		if (src.is_immediate()) {
			uint8_t src_val = src.offset;

			if (src_val == 1) {
				put_inst_std(0b110100, dst, inst, false, dst_wide);
			} else {
				put_inst_std(0b110000, dst, inst, false, dst_wide);
				put_byte(src_val);
			}

			return;
		}

		throw std::runtime_error {"Invalid operands"};

	}

	void BufferWriter::put_inst_tuple(Location dst, Location src, uint8_t opcode_rmr, uint8_t opcode_reg) {

		if (dst.is_simple() && src.is_memreg()) {
			put_inst_std(opcode_rmr, src, dst.base.reg, true, dst.is_wide());
			return;
		}

		if (src.is_simple() && dst.reference) {
			put_inst_std(opcode_rmr, src, dst.base.reg, false, src.is_wide());
			return;
		}

		if (dst.is_memreg() && src.is_immediate()) {
			put_inst_std(0b100000, dst, opcode_reg, false /* TODO: sign field (???) */, dst.is_wide());
			put_inst_label_imm(src, dst.size);
			return;
		}

		throw std::runtime_error {"Invalid operands"};

	}

	/**
	 * Used for constructing the Bit Test family of instructions
	 */
	void BufferWriter::put_inst_btx(Location dst, Location src, uint8_t opcode, uint8_t inst) {

		if (dst.size == WORD || dst.size == DWORD) {
			if (dst.is_memreg() && src.is_simple()) {
				put_inst_std(opcode, dst, src.base.reg, true, true, true);
				return;
			}

			if (dst.is_memreg() && src.is_immediate()) {
				put_inst_std(0b10111010, dst, inst, true);
				put_byte(src.offset);
				return;
			}
		}

		throw std::runtime_error {"Invalid operands"};

	}

	/**
	 * Used for constructing the 'conditional jump' family of instructions
	 */
	void BufferWriter::put_inst_jx(Location dst, uint8_t sopcode, uint8_t lopcode) {

		Label label = dst.label;
		long shift = dst.offset;

		if (!dst.is_jump_label()) {
			throw std::runtime_error {"Invalid operand"};
		}

		if (has_label(label)) {
			long offset = get_label(label) - buffer.size() + shift;

			if (offset > -127) {
				put_byte(sopcode);
				put_label(label, BYTE, shift);
				return;
			}
		}

		put_byte(0b00001111);
		put_byte(lopcode);
		put_label(label, DWORD, shift);

	}

	/**
	 * Used for constructing the 'set byte' family of instructions
	 */
	void BufferWriter::put_inst_setx(Location dst, uint8_t lopcode) {
		put_inst_std(0b1001'0000 | lopcode, dst, 0, true);
	}

	void BufferWriter::put_inst_16bit_operand_mark() {
		put_byte(0b01100110);
	}

	void BufferWriter::put_inst_16bit_address_mark() {
		put_byte(0b01100111);
	}

	void BufferWriter::put_label(const Label& label, uint8_t size, long shift) {
		commands.emplace_back(label, size, buffer.size(), shift, true);

		while (size --> 0) {
			put_byte(0);
		}
	}

	bool BufferWriter::has_label(const Label& label) {
		return labels.contains(label);
	}

	int BufferWriter::get_label(const Label& label) {
		try {
			return labels.at(label);
		} catch (...) {
			throw std::runtime_error {std::string {"Undefined label '"} + label.c_str() + "' used"};
		}
	}

	BufferWriter& BufferWriter::label(const Label& label) {
		if (has_label(label)) {
			throw std::runtime_error {"Invalid label, redefinition attempted"};
		}

		labels[label] = buffer.size();
		return *this;
	}

	void BufferWriter::put_byte(uint8_t byte) {
		buffer.push_back(byte);
	}

	void BufferWriter::put_byte(std::initializer_list<uint8_t> bytes) {
		insert_buffer(buffer, (uint8_t*) std::data(bytes), BYTE * bytes.size());
	}

	void BufferWriter::put_ascii(const std::string& str) {
		insert_buffer(buffer, (uint8_t*) str.c_str(), BYTE * (str.size() + 1));
	}

	void BufferWriter::put_word(uint16_t word) {
		insert_buffer(buffer, (uint8_t*) &word, WORD);
	}

	void BufferWriter::put_word(std::initializer_list<uint16_t> words) {
		insert_buffer(buffer, (uint8_t*) std::data(words), WORD * words.size());
	}

	void BufferWriter::put_dword(uint32_t dword) {
		insert_buffer(buffer, (uint8_t*) &dword, DWORD);
	}

	void BufferWriter::put_dword(std::initializer_list<uint32_t> dwords) {
		insert_buffer(buffer, (uint8_t*) std::data(dwords), DWORD * dwords.size());
	}

	void BufferWriter::put_dword_f(float dword) {
		insert_buffer(buffer, (uint8_t*) &dword, DWORD);
	}

	void BufferWriter::put_dword_f(std::initializer_list<float> dwords) {
		insert_buffer(buffer, (uint8_t*) std::data(dwords), DWORD * dwords.size());
	}

	void BufferWriter::put_qword(uint64_t qword) {
		insert_buffer(buffer, (uint8_t*) &qword, QWORD);
	}

	void BufferWriter::put_qword(std::initializer_list<uint64_t> dwords) {
		insert_buffer(buffer, (uint8_t*) std::data(dwords), QWORD * dwords.size());
	}

	void BufferWriter::put_qword_f(double qword) {
		insert_buffer(buffer, (uint8_t*) &qword, QWORD);
	}

	void BufferWriter::put_qword_f(std::initializer_list<double> dwords) {
		insert_buffer(buffer, (uint8_t*) std::data(dwords), QWORD * dwords.size());
	}

	void BufferWriter::dump(bool verbose) const {
		int i = 0;

		if (verbose) {
			for (uint8_t byte: buffer) {
				std::bitset<8> bin(byte);
				std::cout << std::setfill('0') << std::setw(4) << i << " | ";
				std::cout << std::setfill('0') << std::setw(2) << std::hex << ((int) byte) << ' ' << bin;
				std::cout << " | " << std::dec << (char) (byte < ' ' || byte > '~' ? '.' : byte) << std::endl;
				i++;
			}
		}

		std::cout << "./unasm.sh \"db ";
		bool first = true;

		for (uint8_t byte : buffer) {
			if (!first) {
				std::cout << ", ";
			}

			first = false;
			std::cout << '0' << std::setfill('0') << std::setw(2) << std::hex << ((int) byte) << "h";
		}

		std::cout << '"' << std::endl;
	}

	void BufferWriter::assemble(size_t absolute, ErrorHandler* reporter, bool debug)  {

		for (LabelCommand command : commands) {
			try {
				const long offset = command.relative ? command.offset + command.size : (-absolute);
				const long imm_val = get_label(command.label) - offset + command.shift;
				const uint8_t *imm_ptr = (uint8_t *) &imm_val;
				memcpy(buffer.data() + command.offset, imm_ptr, command.size);
			} catch (std::runtime_error& error) {
				if (reporter != nullptr) reporter->link(command.offset, error.what()); else throw error;
			}
		}

		#if DEBUG_MODE
		debug = true;
		#endif

		if (debug) {
			dump(true);
		}

	}

	ExecutableBuffer BufferWriter::bake(bool debug) {
		ExecutableBuffer result {buffer.size(), labels};
		assemble((size_t) result.address(), nullptr, debug);
		result.bake(buffer);

		return result;
	}

	ElfBuffer BufferWriter::bake_elf(ErrorHandler* reporter, uint32_t address, const char* entry, bool debug) {
		if (!has_label(entry)) {
			throw std::runtime_error {"No entrypoint '" + std::string {entry} + "' defined!"};
		}

		ElfBuffer elf {buffer.size(), address, (uint32_t) labels.at(entry)};
		assemble(address + elf.offset(), reporter, debug);
		elf.bake(buffer);

		return elf;
	}

}