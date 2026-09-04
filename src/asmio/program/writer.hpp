#pragma once

#include <asmio/external.hpp>
#include "label.hpp"
#include "segmented.hpp"

namespace asmio {

	class BasicBufferWriter {

		protected:

			SegmentedBuffer& buffer;

		public:

			constexpr operator bool() const {
				return true;
			}

			BasicBufferWriter(SegmentedBuffer& buffer);

			/// Begin (or continue) writing to a section with the given memory flags
			BasicBufferWriter& section(MemoryFlags flags, const std::string& name = "");

			/// Insert label at the current position of the current section
			BasicBufferWriter& label(const Label& label);

			/// Allow for the given symbol to be missing, create relocations when exporting to an object file
			BasicBufferWriter& import_symbol(const Label& name);

			/// Exported symbols will be able to be resolved from outside the unit when exporting to an object file
			BasicBufferWriter& export_symbol(const Label& label, ExportSymbol::Type type = ExportSymbol::PUBLIC, size_t size = 0);

			void put_cstr(const char* str);
			void put_cstr(const std::string& str);
			void put_byte(uint8_t byte = 0);
			void put_byte(std::initializer_list<uint8_t> byte);
			void put_word(uint16_t word = 0);
			void put_word(std::initializer_list<uint16_t> word);
			void put_dword(uint32_t dword = 0);
			void put_dword(std::initializer_list<uint32_t> dword);
			void put_dword_f(float dword);
			void put_dword_f(std::initializer_list<float> dword);
			void put_qword(uint64_t dword = 0);
			void put_qword(std::initializer_list<uint64_t> dword);
			void put_qword_f(double dword);
			void put_qword_f(std::initializer_list<double> dword);
			void put_data(size_t bytes, const void* data);
			void put_space(size_t bytes, uint8_t value = 0);

	};

}

