#include "lines.hpp"
#include "form.hpp"

namespace asmio {

	/*
	 * DwarfLineEmitter
	 */

	uint8_t DwarfLineEmitter::get_argument_count(uint8_t opcode) {
		if (opcode >= 2 && opcode <= 5) return 1;
		if (opcode == 9 || opcode == 12) return 1;

		return 0;
	}

	bool DwarfLineEmitter::try_emit_special_opcode(int line_offset, unsigned int address_addend) const {
		const size_t opcode = (line_offset - line_base) + (line_range * address_addend) + opcode_base;

		if (opcode <= MAX_OPCODE) {
			body->put<uint8_t>(opcode);
			return true;
		}

		return false;
	}

	void DwarfLineEmitter::emit_address_advance(size_t advance) {

		// this is only more compact if advance is between
		// 14 and 16 bits, between 8 and 13 this uses the same
		// space as LEB128 (IDK why this is even a thing tbh)
		if (advance > 0x7f && advance <= 0xFFFF) {
			emit(DwarfLineStdOpcode::fixed_advance_pc, make_encodec<uint16_t>(advance));
			return;
		}

		emit(DwarfLineStdOpcode::advance_pc, make_encodec<UnsignedLeb128>(advance));
	}

	DwarfLineEmitter::DwarfLineEmitter(ChunkBuffer::Ptr& buffer, int address_bytes)
		: DwarfArrayEmitter(buffer, address_bytes) {

		// DWARF Line Number Program describes a table with one row for each program address
		// that maps that address a specific line and column in one source file.
		// In order to save space this table is not saved as-is, instead constructed dynamically
		// by executing a series of Line Number Program Instructions, those instruction come from 4 groups:
		//
		// - Standard Opcodes - those are defined by DWARF and execute a well-defined function, each takes
		//   some number of LEB128 arguments (except for fixed_advance_pc). They use opcodes starting with 1.
		//
		// - Vendor Specific Opcodes - those use opcodes starting right after the last Standard Opcode and
		//   before the first Special Opcode (see below), their function is vendor specific.
		//
		// - Special Opcodes - there can be a user-defined number of additional opcodes whose function is
		//   defined by a fixed formula. The first Special Opcode uses the opcode set in `opcode_base`.
		//   special_opcode = (line_increment - line_base) + (line_range * address_advance) + opcode_base,
		//   if the resulting special_opcode is larger than MAX_OPCODE a Standard Opcode must be used instead.
		//
		// - Extended Opcodes - Those use a multibyte format. The first byte is 0 (note that Standard Opcodes
		//   have opcodes >0), followed by a ULEB128 describing the number of following bytes. That is then
		//   followed by a single byte extended opcode and any required arguments in opcode-specific format.

		ChunkBuffer::Ptr header = std::make_shared<ChunkBuffer>();

		// after the standard header part (written in DwarfArrayEmitter)
		// comes another length - the number of bytes until the content starts
		head->link<uint32_t>([=] { return header->size(); });
		head->adopt(header);

		// Number of bytes per instruction, in practice this is used
		// by some of the following DWARF opcodes as a multiplier. But
		// to keep things simple we can just safely always say 1 here.
		header->put<uint8_t>(1);

		// Number of VLIW operations per instruction, this is used for
		// so called VLIW architectures. As none of those are supported
		// nor planed to be supported we can set this to 1.
		header->put<uint8_t>(1);

		// Default value for the "is_statement" DWARF register
		header->put<uint8_t>(true);
		state.is_statement = true;

		// Store Special Opcodes configuration
		header->put<int8_t>(line_base);
		header->put<uint8_t>(line_range);
		header->put<uint8_t>(opcode_base);

		// Here we tell DWARF about the argument counts for each standard (and vendor
		// specific) opcode. This allows the DWARF consumer to skip them when reading.
		// See DWARF specification (section 6.2.5.2, page 126)
		for (int i = 1; i < opcode_base; i ++) {
			header->put<uint8_t>(get_argument_count(i));
		}

		// directory table entry format, starting with attribute count
		header->put<uint8_t>(1);
		header->put<UnsignedLeb128>(DwarfLineContent::path);
		header->put<UnsignedLeb128>(DwarfForm::string);

		header->adopt(directory_count);
		header->adopt(directory_list);

		// file table entry format, starting with attribute count
		header->put<uint8_t>(2);
		header->put<UnsignedLeb128>(DwarfLineContent::path);
		header->put<UnsignedLeb128>(DwarfForm::string);
		header->put<UnsignedLeb128>(DwarfLineContent::directory_index);
		header->put<UnsignedLeb128>(DwarfForm::udata);

		header->adopt(file_count);
		header->adopt(file_list);
	}

	DwarfDir DwarfLineEmitter::add_directory(const std::string& path) {
		const DwarfDir dir {directories};

		directories ++;
		directory_list->write(path);

		// rewrite length
		directory_count->clear();
		directory_count->put<UnsignedLeb128>(directories);

		return dir;
	}

	DwarfFile DwarfLineEmitter::add_file(DwarfDir dir, const std::string& path) {
		const DwarfFile file {files};

		files ++;
		file_list->write(path);
		file_list->put<UnsignedLeb128>(dir.handle());

		// rewrite length
		file_count->clear();
		file_count->put<UnsignedLeb128>(files);

		return file;
	}

	void DwarfLineEmitter::set_file(DwarfFile file) {
		if (state.file != file.handle()) {
			state.file = file.handle();

			emit(DwarfLineStdOpcode::set_file, make_encodec<UnsignedLeb128>(state.file));
		}
	}

	void DwarfLineEmitter::set_mapping(size_t address, int line, int column) {

		if (address < state.address) {
			const auto location = std::make_tuple<int, uint64_t>(8, static_cast<uint64_t>(address));
			emit(DwarfLineExtOpcode::set_address, make_encodec<DynamicInt>(location));
			state.address = address;
		}

		if (column != state.column) {
			emit(DwarfLineStdOpcode::set_column, make_encodec<UnsignedLeb128>(column));
			state.column = column;
		}

		auto address_delta = address - state.address;
		auto line_delta = line - state.line;

		if (line_delta > line_range || line_delta < line_base) {
			emit(DwarfLineStdOpcode::advance_line, make_encodec<SignedLeb128>(line - state.line));
			state.line = line;
		}

		if (address_delta > max_advance) {
			emit_address_advance(address_delta);
			state.address = address;
		}

		if (address == state.address && line == state.line) {
			emit(DwarfLineStdOpcode::copy);
			return;
		}

		// (0, 0) will still emit a valid opcode here, but it would have been confusing.
		// also do not that above we tried only encoding if we fall outside special range
		// to try to always use a special opcode if possible (even for only once "channel")
		if (try_emit_special_opcode(line - state.line, address - state.address)) {
			state.line = line;
			state.address = address;
			return;
		}

		// fallback if we fall at those few addresses the above doesn't cover
		// See table in DWARF specification v5 section D.5.2, page 322 - the last row is not full
		if (address != state.address) {
			emit_address_advance(address - state.address);
			state.address = address;
		}

		if (line != state.line) {
			emit(DwarfLineStdOpcode::advance_line, make_encodec<UnsignedLeb128>(line - state.line));
			state.line = line;
		}

		emit(DwarfLineStdOpcode::copy);
	}

	void DwarfLineEmitter::end_sequence() {
		emit(DwarfLineExtOpcode::end_sequence);

		// reinitialize state
		state = {};
	}

}