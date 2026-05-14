#include <asmio/riscv/module.hpp>
#include <asmio/riscv/writer.hpp>

#include "test.hpp"
#include "vstl.hpp"

namespace test {

	using namespace asmio;
	using namespace asmio::riscv;

	TEST (rv32i_check_add) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_add(X1, X7, 13);

		segmented.link(0);
		std::vector<uint8_t> s0 = {0x93, 0x80, 0xd3, 0x00};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	};

	TEST (rv32i_check_basic_op_imm) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_add(X0, X4, 12);
		writer.put_xor(X4, X11, 7);
		writer.put_or(X11, X12, 77);
		writer.put_and(X12, X11, 88);
		writer.put_sll(X11, X3, 7);
		writer.put_srl(X3, X7, 4);
		writer.put_sra(X7, X1, 3);
		writer.put_slt(X1, X12, 3);
		writer.put_sltu(X12, X2, 1000);

		segmented.link(0);
		std::vector<uint8_t> s0 = {0x13, 0x00, 0xc2, 0x00, 0x13, 0xc2, 0x75, 0x00, 0x93, 0x65, 0xd6, 0x04, 0x13, 0xf6, 0x85, 0x05, 0x93, 0x95, 0x71, 0x00, 0x93, 0xd1, 0x43, 0x00, 0x93, 0xd3, 0x30, 0x40, 0x93, 0x20, 0x36, 0x00, 0x13, 0x36, 0x81, 0x3e};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	};

	TEST (rv32i_check_basic_op_reg) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_add(X0, X4, X22);
		writer.put_sub(X0, X4, X22);
		writer.put_xor(X4, X11, X22);
		writer.put_or(X11, X12, X22);
		writer.put_and(X12, X11, X22);
		writer.put_sll(X11, X3, X22);
		writer.put_srl(X3, X7, X22);
		writer.put_sra(X7, X1, X22);
		writer.put_slt(X1, X12, X22);
		writer.put_sltu(X12, X2, X22);

		segmented.link(0);
		std::vector<uint8_t> s0 = {0x33, 0x00, 0x62, 0x01, 0x33, 0x00, 0x62, 0x41, 0x33, 0xc2, 0x65, 0x01, 0xb3, 0x65, 0x66, 0x01, 0x33, 0xf6, 0x65, 0x01, 0xb3, 0x95, 0x61, 0x01, 0xb3, 0xd1, 0x63, 0x01, 0xb3, 0xd3, 0x60, 0x41, 0xb3, 0x20, 0x66, 0x01, 0x33, 0x36, 0x61, 0x01};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	};

	TEST (rv32i_check_basic_loads) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_lb(X0, X5, 100);
		writer.put_lh(X1, X6, 200);
		writer.put_lw(X2, X7, 300);
		writer.put_lbu(X3, X8, 400);
		writer.put_lhu(X4, X9, 500);

		segmented.link(0);
		std::vector<uint8_t> s0 = {0x03, 0x80, 0x42, 0x06, 0x83, 0x10, 0x83, 0x0c, 0x03, 0xa1, 0xc3, 0x12, 0x83, 0x41, 0x04, 0x19, 0x03, 0xd2, 0x44, 0x1f};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	};

	TEST (rv32i_check_basic_stores) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_sb(X1, X4, 0x100);
		writer.put_sh(X2, X5, 0x200);
		writer.put_sw(X3, X6, 0x300);

		// FIXME
		SKIP("Invalid decode");

	};

}