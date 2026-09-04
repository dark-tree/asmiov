#include <asmio/riscv/module.hpp>
#include <asmio/riscv/writer.hpp>
#include <asmio/program/executable.hpp>

#include "test.hpp"
#include "vstl.hpp"

namespace test {

	using namespace asmio;
	using namespace asmio::riscv;

	TEST (rv32i_check_nop) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_nop();

		segmented.link(0);
		std::vector<uint8_t> s0 = {0x13, 0x00, 0x00, 0x00};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	}

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
		writer.put_lw(X1, X6, 200);
		writer.put_ld(X2, X7, 300);
		writer.put_lbu(X3, X8, 400);
		writer.put_lwu(X4, X9, 500);
		writer.put_ldu(X4, X9, 600);
		writer.put_lq(X4, X9, 700);

		segmented.link(0);
		std::vector<uint8_t> s0 = {0x03, 0x80, 0x42, 0x06, 0x83, 0x10, 0x83, 0x0c, 0x03, 0xa1, 0xc3, 0x12, 0x83, 0x41, 0x04, 0x19, 0x03, 0xd2, 0x44, 0x1f, 0x03, 0xe2, 0x84, 0x25, 0x03, 0xb2, 0xc4, 0x2b};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	};

	TEST (rv32i_check_basic_stores) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_sb(X1, X4, 0x100);
		writer.put_sw(X2, X5, 0x200);
		writer.put_sd(X3, X6, 0x300);
		writer.put_sq(X4, X7, 0x400);

		segmented.link(0);
		std::vector<uint8_t> s0 = {0x23, 0x00, 0x12, 0x10, 0x23, 0x90, 0x22, 0x20, 0x23, 0x20, 0x33, 0x30, 0x23, 0xb0, 0x43, 0x40};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	};

	TEST (rv32i_check_ecall_ebreak) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_ecall();
		writer.put_ebreak();

		segmented.link(0);
		std::vector<uint8_t> s0 = {0x73, 0x00, 0x00, 0x00, 0x73, 0x00, 0x10, 0x00};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	};

	TEST (rv32i_check_branch) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_beq(X2, X2, "end");
		writer.put_and(X1, X0, X0);
		writer.label("start");
		writer.put_add(X1, X1, 10);
		writer.put_bgt(X2, X1, "start");
		writer.label("end");

		segmented.link(0);
		std::vector<uint8_t> s0 = {0x63, 0x08, 0x21, 0x00, 0xb3, 0x70, 0x00, 0x00, 0x93, 0x80, 0xa0, 0x00, 0xe3, 0xce, 0x20, 0xfe};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	};

	TEST (rv32i_check_branch_inverted) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.label("test");
		writer.put_b(Condition::GE, X10, X11, "test");
		writer.put_b(Condition::LE, X10, X11, "test");
		writer.put_b(Condition::GTU, X10, X11, "test");

		segmented.link(0);
		std::vector<uint8_t> s0 = {0x63, 0x50, 0xb5, 0x00, 0xe3, 0xde, 0xa5, 0xfe, 0xe3, 0xec, 0xa5, 0xfe};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	};

	TEST (rv32i_check_jump) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.label("start");
		writer.put_add(X1, X1, 10);
		writer.put_jal("start");
		writer.put_jal(X8, "start");
		writer.put_jalr(X6, X7);

		segmented.link(0);
		std::vector<uint8_t> s0 = {0x93, 0x80, 0xa0, 0x00, 0xef, 0xf0, 0xdf, 0xff, 0x6f, 0xf4, 0x9f, 0xff, 0x67, 0x83, 0x03, 0x00};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	};

	TEST (rv32i_check_lui) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_lui(X1, 1);

		segmented.link(0);
		std::vector<uint8_t> s0 = {0xb7, 0x10, 0x00, 0x00};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	};

	TEST (rv32i_check_aliases) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_mov(X11, X12);
		writer.put_neg(X12, X13);
		writer.put_not(X12, X13);
		writer.put_ret();

		segmented.link(0);
		std::vector<uint8_t> s0 = {0x93, 0x05, 0x06, 0x00, 0x33, 0x06, 0xd0, 0x40, 0x13, 0xc6, 0xf6, 0xff, 0x67, 0x80, 0x00, 0x00};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	}

	TEST (rv32m_check) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_mul(X1, X2, X3);
		writer.put_mulh(X1, X2, X3);
		writer.put_mulhsu(X1, X2, X3);
		writer.put_mulhu(X1, X2, X3);
		writer.put_div(X1, X2, X3);
		writer.put_divu(X1, X2, X3);
		writer.put_rem(X1, X2, X3);
		writer.put_remu(X1, X2, X3);

		segmented.link(0);
		std::vector<uint8_t> s0 = {0xb3, 0x00, 0x31, 0x02, 0xb3, 0x10, 0x31, 0x02, 0xb3, 0x20, 0x31, 0x02, 0xb3, 0x30, 0x31, 0x02, 0xb3, 0x40, 0x31, 0x02, 0xb3, 0x50, 0x31, 0x02, 0xb3, 0x60, 0x31, 0x02, 0xb3, 0x70, 0x31, 0x02};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	};

	TEST (rv64m_check) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_muld(X1, X2, X3);
		writer.put_divd(X1, X2, X3);
		writer.put_divud(X1, X2, X3);
		writer.put_remd(X1, X2, X3);
		writer.put_remud(X1, X2, X3);

		segmented.link(0);
		std::vector<uint8_t> s0 = {0xbb, 0x00, 0x31, 0x02, 0xbb, 0x40, 0x31, 0x02, 0xbb, 0x50, 0x31, 0x02, 0xbb, 0x60, 0x31, 0x02, 0xbb, 0x70, 0x31, 0x02};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	};

	TEST (rv64a_check) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_lr(T0, T1, DWORD, Order::ACQUIRE);
		writer.put_lr(T6, T5, QWORD, Order::RELEASE);
		writer.put_sc(T0, T1, T2, DWORD, Order::NONE);

		writer.put_amoswap(T0, T1, T2, DWORD, Order::ACQUIRE);
		writer.put_amoadd(T0, T1, T2, QWORD, Order::RELEASE);
		writer.put_amoand(T0, T1, T2, DWORD, Order::ACQUIRE_RELEASE);
		writer.put_amoor(T0, T1, T2, QWORD, Order::NONE);
		writer.put_amoxor(T0, T1, T2, DWORD, Order::ACQUIRE);
		writer.put_amomax(T0, T1, T2, QWORD, Order::RELEASE);
		writer.put_amomin(T0, T1, T2, DWORD, Order::ACQUIRE_RELEASE);
		writer.put_amomaxu(T0, T1, T2, QWORD, Order::NONE);
		writer.put_amominu(T0, T1, T2, DWORD, Order::ACQUIRE);

		EXPECT_THROW(std::runtime_error) {
			writer.put_amomin(T0, T1, T2, WORD, Order::NONE);
		};

		EXPECT_THROW(std::runtime_error) {
			writer.put_amomin(T0, T1, T2, BYTE, Order::NONE);
		};

		segmented.link(0);
		std::vector<uint8_t> s0 = {0xaf, 0x22, 0x03, 0x14, 0xaf, 0x3f, 0x0f, 0x12, 0xaf, 0x22, 0x73, 0x18, 0xaf, 0x22, 0x73, 0x0c, 0xaf, 0x32, 0x73, 0x02, 0xaf, 0x22, 0x73, 0x66, 0xaf, 0x32, 0x73, 0x40, 0xaf, 0x22, 0x73, 0x24, 0xaf, 0x32, 0x73, 0xa2, 0xaf, 0x22, 0x73, 0x86, 0xaf, 0x32, 0x73, 0xe0, 0xaf, 0x22, 0x73, 0xc4};
		CHECK(segmented.segments()[0].buffer, s0); // .rwx

	};

	/*
	 * region Executable
	 * Begin architecture depended tests for Risc-V
	 */

#if ARCH_RISCV64

	TEST (riscv_exec_leaf_function) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_add(A0, X0, 42);
		writer.put_ret();

		auto exe = to_executable(segmented);
		CHECK(exe.call_u64(), 42);

	};

	TEST (riscv_exec_nop_neg) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_nop();
		writer.put_add(T0, X0, 7);
		writer.put_neg(A0, T0);
		writer.put_ret();

		auto exe = to_executable(segmented);
		CHECK(exe.call_i64(), -7);

	};

	TEST (riscv_exec_jump) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_j("skip_30");
		writer.put_add(A0, X0, 30);
		writer.put_ret();

		writer.label("skip_30");
		writer.put_add(A0, X0, 40);
		writer.put_ret();

		auto exe = to_executable(segmented);
		CHECK(exe.call_u64(), 40);

	};

	TEST (riscv_exec_multiply) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_add(T0, X0, 11);
		writer.put_add(T1, X0, 4);
		writer.put_add(T2, X0, 3);
		writer.put_or(T1, T1, T2);
		writer.put_mul(A0, T0, T1);
		writer.put_ret();

		auto exe = to_executable(segmented);
		CHECK(exe.call_u64(), 77);

	};

	TEST (riscv_exec_lui) {
		SegmentedBuffer segmented;
		BufferWriter writer {segmented};
		segmented.elf_machine = ElfMachine::RISCV;

		writer.put_or(A0, X0, 42);
		writer.put_lui(A0, 0xfffff);
		writer.put_sll(A0, A0, 4);
		writer.put_add(A0, A0, 0x7ff);
		writer.put_ret();

		auto exe = to_executable(segmented);
		CHECK(exe.call_u64(), 0xffffffffffff07ff);
	};

	TEST (riscv_exec_mov_imm) {

		auto verify = [] (uint64_t value, uint32_t instructions) {

			SegmentedBuffer segmented;
			BufferWriter writer {segmented};
			segmented.elf_machine = ElfMachine::RISCV;

			writer.put_or(A0, X0, 0xABC);
			const uint32_t start = segmented.current().offset;
			writer.put_mov(A0, value);
			const uint32_t end = segmented.current().offset;

			writer.put_ret();

			auto exe = to_executable(segmented);

			// "0x00" added as VSTL formatting hint
			CHECK(exe.call_u64(), 0x00 + value);
			CHECK((end - start), (instructions * 4));

		};

		verify(0, 1);
		verify(1, 1);
		verify(-1, 1);
		verify(0x7ff, 1);
		verify(0xfff, 2);
		verify(0x8bcd'1234, 4);
		verify(0x7bcd'1234, 2);
		verify(0x7bcd'7fff, 2);
		verify(0x1234'1234'1234'1234, 8);
		verify(0xffff'ffff'8223'4523, 2);
		verify(0xffff'ff12'3452'3123, 4);
		verify(0x5555'0000'0044'4444, 5);
		verify(0x66666'000, 1);
		verify(0x1000'0000'0000'0001, 3);

	};

	TEST (riscv_exec_mov_fuzzer) {

		for (int i = 0; i < 5000; i ++) {
			uint64_t value = vstl_self.random.next();

			SegmentedBuffer segmented;
			BufferWriter writer {segmented};
			segmented.elf_machine = ElfMachine::RISCV;

			writer.put_and(A0, X0, 0);
			writer.put_mov(A0, value);
			writer.put_ret();

			auto exe = to_executable(segmented);

			// "0x00" added as VSTL formatting hint
			CHECK(exe.call_u64(), 0x00 + value);
			CHECK(exe.call_u64(), 0x00 + value);
		}

	};

	TEST (riscv_exec_external_call) {

		int (*function) () = [] () {
			return 42;
		};

		SegmentedBuffer buffer;
		BufferWriter writer(buffer);

		writer.put_add(SP, SP, -16);
		writer.put_sq(RA, SP, 8);
		writer.put_mov(T0, reinterpret_cast<uint64_t>(function));
		writer.put_jalr(RA, T0);
		writer.put_lq(RA, SP, 8);
		writer.put_add(SP, SP, 16);
		writer.put_jr(RA);

		ExecutableBuffer exe = to_executable(buffer);
		CHECK(exe.call_u64(), 42);

	};

	TEST (riscv_exec_external_tail_call) {

		int (*function) () = [] () {
			return 42;
		};

		SegmentedBuffer buffer;
		BufferWriter writer(buffer);

		writer.put_mov(T0, reinterpret_cast<uint64_t>(function));
		writer.put_jr(T0);

		ExecutableBuffer exe = to_executable(buffer);
		CHECK(exe.call_u64(), 42);

	};

	TEST (riscv_exec_lookup_symbol_forward) {

		SegmentedBuffer buffer;
		BufferWriter writer(buffer);

		writer.label("start");
		writer.put_mov(T0, "var"); // look forward
		writer.put_lq(A0, T0);
		writer.put_add(A0, A0, -0x11);
		writer.put_ret();

		writer.put_nop();
		writer.put_nop();
		writer.put_nop();
		writer.put_nop();

		writer.label("var");
		writer.put_qword(0x54);

		ExecutableBuffer exe = to_executable(buffer);
		CHECK(exe.call_u64("start"), 0x43);

	};

	TEST (riscv_exec_lookup_symbol_back) {

		SegmentedBuffer buffer;
		BufferWriter writer(buffer);

		writer.label("var");
		writer.put_qword(0x54);

		writer.put_nop();
		writer.put_nop();
		writer.put_nop();
		writer.put_nop();

		writer.label("start");
		writer.put_mov(T0, "var"); // look back
		writer.put_lq(A0, T0);
		writer.put_add(A0, A0, -0x12);
		writer.put_ret();

		ExecutableBuffer exe = to_executable(buffer);
		CHECK(exe.call_u64("start"), 0x42);

	};

#endif

}