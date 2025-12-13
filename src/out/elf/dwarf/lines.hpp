#pragma once
#include <out/chunk/codecs.hpp>

#include "emitter.hpp"

namespace asmio {

	class DwarfLineEmitter;

	struct DwarfDir : util::UniqueHandle<DwarfLineEmitter, int> {};
	struct DwarfFile : util::UniqueHandle<DwarfLineEmitter, int> {};

	/// See DWARF specification v5 section 7.22, page 236
	/// DWARF Standard Line Number Program opcodes
	enum struct DwarfLineStdOpcode {
		extension          = 0x00, // begin DwarfLineExtOpcode
		copy               = 0x01, //
		advance_pc         = 0x02, // UnsignedLeb128
		advance_line       = 0x03, // SignedLeb128
		set_file           = 0x04, // UnsignedLeb128
		set_column         = 0x05, // UnsignedLeb128
		negate_stmt        = 0x06, //
		set_basic_block    = 0x07, //
		const_add_pc       = 0x08, //
		fixed_advance_pc   = 0x09, // uint8_t
		set_prologue_end   = 0x0a, //
		set_epilogue_begin = 0x0b, //
		set_isa            = 0x0c, // UnsignedLeb128
	};

	/// See DWARF specification v5 section 7.22, page 237
	/// DWARF Extended Line Number Program opcodes
	enum struct DwarfLineExtOpcode {
		end_sequence       = 0x01, //
		set_address        = 0x02, // address of the target architecture
		set_discriminator  = 0x04, // UnsignedLeb128
	};

	/// See DWARF specification v5 section 7.22, page 237
	/// DWARF Standard Content Descriptions for the Line Number Program
	enum struct DwarfLineContent {
		path               = 0x01,
		directory_index    = 0x02,
		timestamp          = 0x03,
		size               = 0x04,
		MD5                = 0x05,
	};

	struct DwarfLineNumberProgramState {
		size_t address = 0;
		int file = -1;
		int line = 1;
		int column = 0;

		bool is_statement = false;
		bool basic_block = false;
		bool end_sequence = false;
		bool prologue_end = false;
		bool epilogue_begin = false;

		// some unused registers were omitted
	};

	/// See DWARF specification v5 section 6.2, page 148
	/// Emitter for the DWARF Line Number Program source mapping
	class DwarfLineEmitter : public DwarfArrayEmitter {

		private:

			int directories = 0;
			int files = 0;

			DwarfLineNumberProgramState state;

		private:

			static constexpr size_t MAX_OPCODE = 255;

			ChunkBuffer::Ptr directory_count = std::make_shared<ChunkBuffer>();
			ChunkBuffer::Ptr directory_list = std::make_shared<ChunkBuffer>();
			ChunkBuffer::Ptr file_count = std::make_shared<ChunkBuffer>();
			ChunkBuffer::Ptr file_list = std::make_shared<ChunkBuffer>();

			// Configuration for Special Opcodes (see below), we do not try to use the best opcode
			// definitions for a given program, only using the special opcodes when they fit our needs.
			// those values here match the ones provided as example in the DWARF specification (section D.5.2, page 321)
			// for convenience and because they provide a "nicely looking" range of values.
			const int line_base = -3;
			const int line_range = 12;
			const int opcode_base = 13; // no vendor extensions

			const size_t max_advance = (MAX_OPCODE - opcode_base) / line_range; // approximation

			/// Get the number of arguments for a particular Standard Opcode
			static uint8_t get_argument_count(uint8_t opcode);

			/// Try encoding a Special Opcode, if the returned value is larger than MAX_OPCODE different encoding should be used
			bool try_emit_special_opcode(int line_offset, unsigned int address_addend) const;

			/// Advance the DWARF address by a constant
			void emit_address_advance(size_t advance);

			template <any_encodec... Args>
			void emit(DwarfLineStdOpcode opcode, Args... args) {
				body->put<uint8_t>(opcode);
				(body->put<typename Args::codec>(args.value), ...);
			}

			template <any_encodec... Args>
			void emit(DwarfLineExtOpcode opcode, Args... args) {
				body->put<uint8_t>(DwarfLineStdOpcode::extension);

				ChunkBuffer::Ptr tmp = std::make_shared<ChunkBuffer>();

				tmp->put<uint8_t>(opcode);
				(tmp->put<typename Args::codec>(args.value), ...);

				body->put<UnsignedLeb128>(tmp->bytes());
				body->merge(tmp);
			}

		public:

			DwarfLineEmitter(ChunkBuffer::Ptr& buffer, int address_bytes);

			/// Add a new directory entry and get its handle
			DwarfDir add_directory(const std::string& path);

			/// Add a new file entry and get its handle
			DwarfFile add_file(DwarfDir dir, const std::string& path);

			/// Associate the given address with the given line and column in the current file
			void set_mapping(size_t address, DwarfFile file, int line, int column);

			/// Terminate the current sequence
			void end_sequence();

	};

}
