#pragma once

#include <asmio/program/writer.hpp>
#include <asmio/riscv/argument/registry.hpp>

namespace asmio::riscv {

	class BufferWriter : public BasicBufferWriter {

		protected:

			void put_inst_r(uint8_t func7, Registry rs2, Registry rs1, uint8_t func3, Registry rd, uint8_t opc7);
			void put_inst_i(uint16_t imm12, Registry rs1, uint8_t func3, Registry rd, uint8_t opc7);
			void put_inst_s(uint16_t imm12, Registry rs2, Registry rs1, uint8_t func3, uint8_t opc7);
			void put_inst_b(uint16_t imm12, Registry rs2, Registry rs1, uint8_t func3, uint8_t opc7);
			void put_inst_u(uint32_t imm20, Registry rd, uint8_t opc7);
			void put_inst_j(uint32_t imm20, Registry rd, uint8_t opc7);

		public:

			BufferWriter(SegmentedBuffer& buffer);

			INST put_add(Registry rd, Registry rs, int16_t imm12); ///< Add signed 12 bit immediate to rs and save result to rd

	};

}