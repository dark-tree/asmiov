
#include "writer.hpp"
#include "sizes.hpp"

namespace asmio {

	BasicBufferWriter::BasicBufferWriter(SegmentedBuffer& buffer)
		: buffer(buffer) {
	}

	BasicBufferWriter& BasicBufferWriter::section(uint8_t flags, const std::string& name) {
		buffer.use_section(flags, name);
		return *this;
	}

	BasicBufferWriter& BasicBufferWriter::label(const Label& label) {
		buffer.add_label(label);
		return *this;
	}

	BasicBufferWriter& BasicBufferWriter::export_symbol(const Label& label, ExportSymbol::Type type, size_t size) {
		buffer.add_export(label, type, size);
		return *this;
	}

	void BasicBufferWriter::put_byte(uint8_t byte) {
		buffer.push(byte);
	}

	void BasicBufferWriter::put_byte(std::initializer_list<uint8_t> bytes) {
		buffer.insert((uint8_t*) std::data(bytes), BYTE * bytes.size());
	}

	void BasicBufferWriter::put_cstr(const char* str) {
		while (*str) {
			put_byte(*(str ++));
		}

		put_byte(0);
	}

	void BasicBufferWriter::put_cstr(const std::string& str) {
		buffer.insert((uint8_t*) str.c_str(), BYTE * (str.size() + 1));
	}

	void BasicBufferWriter::put_word(uint16_t word) {
		buffer.insert((uint8_t*) &word, WORD);
	}

	void BasicBufferWriter::put_word(std::initializer_list<uint16_t> words) {
		buffer.insert((uint8_t*) std::data(words), WORD * words.size());
	}

	void BasicBufferWriter::put_dword(uint32_t dword) {
		buffer.insert((uint8_t*) &dword, DWORD);
	}

	void BasicBufferWriter::put_dword(std::initializer_list<uint32_t> dwords) {
		buffer.insert((uint8_t*) std::data(dwords), DWORD * dwords.size());
	}

	void BasicBufferWriter::put_dword_f(float dword) {
		buffer.insert((uint8_t*) &dword, DWORD);
	}

	void BasicBufferWriter::put_dword_f(std::initializer_list<float> dwords) {
		buffer.insert((uint8_t*) std::data(dwords), DWORD * dwords.size());
	}

	void BasicBufferWriter::put_qword(uint64_t qword) {
		buffer.insert((uint8_t*) &qword, QWORD);
	}

	void BasicBufferWriter::put_qword(std::initializer_list<uint64_t> dwords) {
		buffer.insert((uint8_t*) std::data(dwords), QWORD * dwords.size());
	}

	void BasicBufferWriter::put_qword_f(double qword) {
		buffer.insert((uint8_t*) &qword, QWORD);
	}

	void BasicBufferWriter::put_qword_f(std::initializer_list<double> dwords) {
		buffer.insert((uint8_t*) std::data(dwords), QWORD * dwords.size());
	}

	void BasicBufferWriter::put_data(size_t bytes, void* date) {
		buffer.insert((uint8_t*) date, bytes);
	}

	void BasicBufferWriter::put_space(size_t bytes, uint8_t value) {
		buffer.fill(bytes, value);
	}

}