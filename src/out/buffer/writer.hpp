#pragma once

#include "external.hpp"
#include "label.hpp"
#include "segmented.hpp"

namespace asmio {

	class BasicBufferWriter {

		protected:

			SegmentedBuffer& buffer;

		public:

			BasicBufferWriter(SegmentedBuffer& buffer);

			BasicBufferWriter& section(uint8_t flags, const std::string& name = "");
			BasicBufferWriter& label(const Label& label);
			BasicBufferWriter& export_symbol(const Label& label);

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
			void put_data(size_t bytes, void* date);
			void put_space(size_t bytes, uint8_t value = 0);

	};

}

