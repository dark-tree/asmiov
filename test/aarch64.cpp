#define DEBUG_MODE false
#define VSTL_TEST_COUNT 3
#define VSTL_PRINT_SKIP_REASON true
#define VSTL_SUBMODULE true

#include "vstl.hpp"
#include "asm/aarch64/writer.hpp"
#include "out/buffer/executable.hpp"
#include <tasml/top.hpp>

namespace test::arm {

	using namespace asmio;
	using namespace asmio::arm;

	/*
	 * region Static
	 * Begin target invariant tests for ARM
	 */

	TEST (writer_fail_movz_invalid) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		EXPECT_THROW(std::runtime_error) {
			writer.put_movz(W(0), 0x102A, 15);
		};

		EXPECT_THROW(std::runtime_error) {
			writer.put_movz(W(0), 0x102A, 32);
		};

		EXPECT_THROW(std::runtime_error) {
			writer.put_movz(X(0), 0x102A, 64);
		};

		EXPECT_THROW(std::runtime_error) {
			writer.put_movz(SP, 0);
		};

	};

	TEST (writer_fail_3reg_invalid) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		EXPECT_THROW(std::runtime_error) {
			writer.put_inst_orr(W(0), W(0), X(0));
		};

	};

	TEST (writer_fail_orr_imm_invalid) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		EXPECT_THROW(std::runtime_error) {
			writer.put_inst_orr(W(0), W(1), 0xFF00FF00'FF00FF00);
		};

		EXPECT_THROW(std::runtime_error) {
			writer.put_inst_orr(X(0), X(1), 0b00100000'00110000);
		};

		EXPECT_THROW(std::runtime_error) {
			writer.put_inst_orr(X(0), X(1), 0);
		};

		EXPECT_THROW(std::runtime_error) {
			writer.put_inst_orr(X(0), X(1), std::numeric_limits<uint64_t>::max());
		};

	};

	TEST (writer_fail_rbit_cls_clz_invalid) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		EXPECT_THROW(std::runtime_error) {
			writer.put_rbit(W(0), X(1));
		};

		EXPECT_THROW(std::runtime_error) {
			writer.put_cls(W(0), X(1));
		};

		EXPECT_THROW(std::runtime_error) {
			writer.put_clz(W(0), X(1));
		};

	};

	TEST (writer_fail_adc_non_generic) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		EXPECT_THROW(std::runtime_error) {
			writer.put_adc(X(0), X(1), SP);
		};

		EXPECT_THROW(std::runtime_error) {
			writer.put_adc(X(0), SP, X(1));
		};

		EXPECT_THROW(std::runtime_error) {
			writer.put_adc(SP, X(0), X(1));
		};

	};

	/*
	 * region Executable
	 * Begin architecture depended tests for ARM
	 */

#if ARCH_AARCH64

	TEST (writer_exec_nop_ret) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_nop();
		writer.put_ret();

		to_executable(segmented).call_u64();

	};

	TEST (writer_exec_movz) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_movz(X(0), 1, 0); // this should be overridden
		writer.put_movz(X(0), 0x102A, 16);
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 0x102A'0000)

	};

	TEST (writer_exec_movk) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_movz(X(0), 1, 0); // this should be overridden
		writer.put_movk(X(0), 0x102A, 16);
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 0x102A'0001)

	};

	TEST (writer_exec_movn) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_movn(X(0), 0x00FF, 16);
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 0xFFFF'FFFF'FF00'FFFF);

	};

	TEST (writer_exec_orr) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_movz(X(4), 0x0021);
		writer.put_movz(X(3), 0x4300);
		writer.put_inst_orr(X(2), X(3), X(4));
		writer.put_inst_orr(X(0), XZR, X(2));
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 0x4321);

	};

	TEST (writer_exec_orr_bitmask) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_movz(X(1), 0b00000101);
		writer.put_inst_orr(X(0), X(1), 0b1001100110011001100110011001100110011001100110011001100110011001);
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 0b1001100110011001100110011001100110011001100110011001100110011101);

	};

	TEST (writer_exec_add) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_movz(X(0), 2);
		writer.put_movz(X(1), 7);
		writer.put_movz(X(2), 11);
		writer.put_add(X(0), X(1), X(2));
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 7 + 11);

	};

	TEST (writer_exec_add_byte) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_movz(X(0), 0);
		writer.put_movz(X(1), 0x0133);
		writer.put_movz(X(2), 0xF011);
		writer.put_add(X(0), X(1), X(2), Sizing::UB);
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 0x0144);

	};

	TEST (writer_exec_mov_ret) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(1), LR);
		writer.put_mov(X(0), 42);
		writer.put_mov(LR, 0);
		writer.put_ret(X(1));

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 42);

	};

	TEST (writer_exec_rbit) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(1), 0xFFF80000'00000000);
		writer.put_rbit(X(0), X(1));
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 0x1FFF);

	};

	TEST (writer_exec_adds_adc) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(0), 0);
		writer.put_mov(X(1), 0xFFFF'FFFF'FFFF'FFFF);
		writer.put_mov(X(2), 41);
		writer.put_mov(X(3), 1);

		writer.put_adds(X(4), X(1), X(3));
		writer.put_adc(X(0), X(2), X(4));
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 42);

	};

	TEST (writer_exec_no_branch) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(1), 3);
		writer.put_mov(X(0), 11);

		writer.put_mov(X(0), 22);
		writer.label("skip_22");
		writer.put_add(X(0), X(0), X(1));
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 25);

	};

	TEST (writer_exec_b) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(1), 3);
		writer.put_mov(X(0), 11);
		writer.put_b("skip_22");
		writer.put_mov(X(0), 22);
		writer.label("skip_22");
		writer.put_add(X(0), X(0), X(1));
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 14);

	};

	TEST (writer_exec_b_cond) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(0), 0);

		writer.put_mov(X(1), 0xFFFF'FFFF'FFFF'FFF0);
		writer.put_mov(X(2), 0x1F);
		writer.put_mov(X(3), 0x0F);

		writer.put_adds(X(4), X(1), X(3));
		writer.put_b(Condition::CS, "fail");

		writer.put_adds(X(4), X(1), X(2));
		writer.put_b(Condition::CS, "success");

		writer.label("fail");
		writer.put_mov(X(0), 11);
		writer.put_ret();

		writer.label("success");
		writer.put_mov(X(0), 22);
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 22);

	};

	TEST (writer_exec_bl_br) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(0), 0);
		writer.put_mov(X(4), LR);
		writer.put_bl("foo");

		writer.put_mov(X(1), 10);
		writer.put_add(X(0), X(0), X(1));
		writer.put_br(X(4)); // return to C++

		writer.label("foo");
		writer.put_mov(X(0), 40);
		writer.put_ret(); // check if we correctly return to BL

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 50);

	};

	TEST (writer_exec_clz) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(1), 0x0000'F000'0000'F000);
		writer.put_clz(X(0), X(1));
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 16);

	};

	TEST (writer_exec_cls_negative) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(1), 0xF000'0000'0000'00FF);
		writer.put_cls(X(0), X(1));
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 3);

	};

	TEST (writer_exec_cls_positive) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(1), 0x0100'0000'0000'00F0);
		writer.put_cls(X(0), X(1));
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 6);

	};

	TEST (writer_exec_cbnz) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(0), 0);
		writer.put_mov(X(1), 1);
		writer.put_cbnz(X(1), "skip");
		writer.put_ret();

		writer.label("skip");
		writer.put_mov(X(0), 0xCB);
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 0xCB);

	};

	TEST (writer_exec_cbz) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(0), 0);
		writer.put_cbz(X(0), "skip");
		writer.put_ret();

		writer.label("skip");
		writer.put_mov(X(0), 0xBC);
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 0xBC);

	};

	TEST (writer_exec_ldr_literal) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.label("alpha");
		writer.put_ldr(X(0), "data");
		writer.put_ret();

		writer.label("beta");
		writer.put_ldr(W(0), "data");
		writer.put_ret();

		writer.label("data");
		writer.put_qword(0xAABB'CCDD'EEFF'1234);

		uint64_t r0 = to_executable(segmented).call_u64("alpha");
		CHECK(r0, 0xAABB'CCDD'EEFF'1234);

		r0 = to_executable(segmented).call_u64("beta");
		CHECK(r0, 0xEEFF'1234);

	};

	TEST (writer_exec_adr) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_adr(X(0), "data");
		writer.put_ret();

		writer.label("data");
		writer.put_dword(0xAABBCCDD);

		auto buffer = to_executable(segmented);
		auto* data = (uint32_t*) buffer.call_u64();
		CHECK(*data, 0xAABBCCDD);

	};

	TEST (writer_exec_ldr_unsigned_byte) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_adr(X(1), "data");
		writer.put_ldr(X(2), X(1), 1, Sizing::UB);
		writer.put_ldr(X(3), X(1), 3, Sizing::UB);
		writer.put_ldr(X(4), X(1), 4, Sizing::UB);
		writer.put_add(X(0), X(2), X(3));
		writer.put_add(X(0), X(0), X(4));
		writer.put_ret();

		writer.label("data");
		writer.put_byte(0b000001); // +0
		writer.put_byte(0b000010); // +1
		writer.put_byte(0b000100); // +2
		writer.put_byte(0b001000); // +3
		writer.put_byte(0b010000); // +4
		writer.put_byte(0b100000); // +5

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 0b011010);

	};

	TEST (writer_exec_ldr_signed_word) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_adr(X(1), "data");
		writer.put_ldr(X(0), X(1), 2, Sizing::SH);
		writer.put_ret();

		writer.label("data");
		writer.put_word(0xAAAA);
		writer.put_word(-13);
		writer.put_word(0xAAAA);

		uint64_t r0 = to_executable(segmented).call_i64();
		CHECK(r0, -13); // sign extended

	};

	TEST (writer_exec_ldr_unsigned_word) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_adr(X(1), "data");
		writer.put_ldr(X(0), X(1), 2, Sizing::UH);
		writer.put_ret();

		writer.label("data");
		writer.put_word(0xAAAA);
		writer.put_word(0xFFFF);
		writer.put_word(0xAAAA);

		uint64_t r0 = to_executable(segmented).call_i64();
		CHECK(r0, 0xFFFF); // not signed extended

	};

	TEST (writer_exec_ldri_unsigned_qword) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_adr(X(1), "data");
		writer.put_mov(X(0), 0);

		writer.put_ldri(X(2), X(1), 8, Sizing::UX);
		writer.put_add(X(0), X(2), X(0));
		writer.put_ldri(X(2), X(1), 8, Sizing::UX);
		writer.put_add(X(0), X(2), X(0));
		writer.put_ldri(X(2), X(1), 8, Sizing::UX);
		writer.put_add(X(0), X(2), X(0));

		writer.put_ret();

		writer.put_word(0xFFFF);

		writer.label("data");
		writer.put_qword(0b0001);
		writer.put_qword(0b0010);
		writer.put_qword(0b0100);
		writer.put_word(0xFFFF);

		uint64_t r0 = to_executable(segmented).call_i64();
		CHECK(r0, 0b0111);

	};

	TEST (writer_exec_ildr_unsigned_dword) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_adr(X(1), "data");
		writer.put_mov(X(0), 0);

		writer.put_ildr(X(2), X(1), 4, Sizing::UW);
		writer.put_add(X(0), X(2), X(0));
		writer.put_ildr(X(2), X(1), 4, Sizing::UW);
		writer.put_add(X(0), X(2), X(0));
		writer.put_ildr(X(2), X(1), 4, Sizing::UW);
		writer.put_add(X(0), X(2), X(0));

		writer.put_ret();

		writer.label("data");
		writer.put_dword(0x0FFFFFFF);
		writer.put_dword(0x100000AA);
		writer.put_dword(0x1000BB00);
		writer.put_dword(0x10CC0000);
		writer.put_dword(0x0FFFFFFF);

		uint64_t r0 = to_executable(segmented).call_i64();
		CHECK(r0, 0x30CCBBAA);

	};

	TEST (writer_exec_ildr_signed_byte) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_adr(X(1), "data");
		writer.put_ildr(X(0), X(1), 3, Sizing::SB);
		writer.put_ret();

		writer.put_word(0xFFFF);

		writer.label("data");
		writer.put_byte(0xAA);
		writer.put_byte(0xAA);
		writer.put_byte(0xAA);
		writer.put_byte(-42);
		writer.put_word(0xFFFF);

		uint64_t r0 = to_executable(segmented).call_i64();
		CHECK(r0, -42);

	};

	TEST (writer_exec_stri_byte) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.label("read");
		writer.put_adr(X(0), "data");
		writer.put_ret();

		writer.label("write");
		writer.put_adr(X(1), "data");
		writer.put_mov(X(2), 9);
		writer.put_mov(X(0), 0xF1);
		writer.put_stri(X(0), X(1), 1, Sizing::SB);

		writer.put_add(X(0), X(0), X(2));
		writer.put_istr(X(0), X(1), 0, Sizing::UB);
		writer.put_ret();

		writer.put_word(0xFFFF);

		writer.label("data");
		writer.put_byte(0);
		writer.put_byte(0);
		writer.put_word(0xFFFF);

		auto buffer = to_executable(segmented);
		buffer.call_i64("write");

		auto* ptr = std::bit_cast<uint8_t*>(buffer.call_i64("read"));
		CHECK(ptr[0], 0xF1);
		CHECK(ptr[1], 0xFA);
		CHECK(ptr[2], 0xFF);
		CHECK(ptr[3], 0xFF);

	};

	TEST (writer_exec_eor) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(1), 0b0111'0101);
		writer.put_mov(X(2), 0b0101'1100);

		writer.put_eor(X(0), X(1), X(2));
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_i64();
		CHECK(r0, 0b0111'0101 ^ 0b0101'1100);

	};

	TEST (writer_exec_orr) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(1), 0b0111'0101);
		writer.put_mov(X(2), 0b0101'1100);

		writer.put_orr(X(0), X(1), X(2));
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_i64();
		CHECK(r0, 0b0111'0101 | 0b0101'1100);

	};

	TEST (writer_exec_and) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(1), 0b0111'0101);
		writer.put_mov(X(2), 0b0101'1100);

		writer.put_and(X(0), X(1), X(2));
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_i64();
		CHECK(r0, 0b0111'0101 & 0b0101'1100);

	};

	TEST (writer_exec_and_shifted) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(1), 0b0111'0101);
		writer.put_mov(X(2), 0b0101'1100);

		writer.put_and(X(0), X(1), X(2), ShiftType::LSR, 3);
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_i64();
		CHECK(r0, 0b0111'0101 & 0b0000101'1);

	};

	TEST (writer_exec_svc_linux_getcwd) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		// https://arm64.syscall.sh/
		writer.label("getcwd");
		writer.put_adr(X(0), "buffer");
		writer.put_mov(X(1), 100);
		writer.put_mov(X(8), 17); // getcwd
		writer.put_svc(0);

		writer.put_adr(X(0), "buffer");
		writer.put_ret();

		writer.label("buffer");
		writer.put_space(128);

		auto executable = to_executable(segmented);
		auto ptr = std::bit_cast<char*>(executable.call_i64("getcwd"));

		char path[128];
		getcwd(path, 100);

		ASSERT(strcmp(path, ptr) == 0);

	};

	TEST (writer_exec_sub) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(1), 100);
		writer.put_mov(X(2), 17);
		writer.put_sub(X(0), X(1), X(2));
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_i64();
		CHECK(r0, 100 - 17);

	};

	TEST (writer_exec_subs) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(1), 2);
		writer.put_mov(X(2), 1);
		writer.put_subs(X(0), X(2), X(1));

		writer.put_b(Condition::NE, "skip_1");
		writer.put_mov(X(0), 0);
		writer.put_ret();
		writer.label("skip_1");

		writer.put_subs(X(0), X(1), X(1));

		writer.put_b(Condition::EQ, "skip_2");
		writer.put_mov(X(0), 0);
		writer.put_ret();
		writer.label("skip_2");

		writer.put_mov(X(0), 22);
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_i64();
		CHECK(r0, 22);

	};

	TEST (writer_exec_cmp) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(1), 22);
		writer.put_mov(X(2), 11);
		writer.put_cmp(X(1), X(2));
		writer.put_b(Condition::GT, "gt");

		writer.put_mov(X(0), 0);
		writer.put_ret();
		writer.label("gt");

		writer.put_mov(X(1), 11);
		writer.put_mov(X(2), 22);
		writer.put_cmp(X(1), X(2));
		writer.put_b(Condition::GT, "bad");

		writer.put_mov(X(0), 33);
		writer.put_ret();

		writer.label("bad");
		writer.put_mov(X(0), 1);
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_i64();
		CHECK(r0, 33);

	};

	TEST (writer_exec_cmn) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_mov(X(1), 22);
		writer.put_mov(X(2), -22);
		writer.put_cmn(X(1), X(2));
		writer.put_b(Condition::EQ, "zero");

		writer.put_mov(X(0), 0);
		writer.put_ret();
		writer.label("zero");

		writer.put_mov(X(1), 0xFFFF'FFFF'FFFF'FFF0);
		writer.put_mov(X(2), 0xFF);
		writer.put_cmn(X(1), X(2));
		writer.put_b(Condition::CC, "bad");

		writer.put_mov(X(0), 44);
		writer.put_ret();

		writer.label("bad");
		writer.put_mov(X(0), 1);
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_i64();
		CHECK(r0, 44);

	};

	TEST (tasml_exec_simple_linux_syscall) {

		std::string code = R"(
			lang aarch64

			section r
			text:
				byte "Hello!", 0

			section rx
			_start:
				mov x0, 100
				mov x8, 0x5d // exit
				svc 0
		)";

		tasml::ErrorHandler reporter {vstl_self.name, true};
		SegmentedBuffer buffer = tasml::assemble(reporter, code);

		asmio::elf::ElfBuffer elf = asmio::elf::to_elf(buffer, "_start", DEFAULT_ELF_MOUNT, [&] (const auto& link, const char* what) {
			reporter.link(link.target, what);
		});

		if (!reporter.ok()) {
			reporter.dump();
			FAIL("Failed to link executable");
		}

		int rc = 0;
		asmio::elf::RunResult result = elf.execute("memfd", &rc);

		CHECK(result, asmio::elf::RunResult::SUCCESS);
		CHECK(rc, 100);

	};

	TEST (tasml_exec_mul_madd_maddl) {

		std::string code = R"(
			lang aarch64

			section rx
			l_madd_1007:
				mov x1, 5
				mov x2, 200
				mov x3, 7
				madd x0, x1, x2, x3
				ret

			l_mul_500:
				mov x1, 5
				mov x2, 100
				mov x3, 7
				mul x0, x1, x2
				ret

			l_umaddl:
				mov w1, 0xFEB00000
				mov w2, 16
				mov x3, 7
				umaddl x0, w1, w2, x3
				ret
		)";

		tasml::ErrorHandler reporter {vstl_self.name, true};
		SegmentedBuffer buffer;

		try {
			buffer = tasml::assemble(reporter, code);
		} catch (std::runtime_error& e) {
			reporter.dump();
			FAIL("Failed to assemble - " + std::string(e.what()));
		}

		uint64_t r0 = to_executable(buffer).call_i64("l_madd_1007");
		CHECK(r0, 1007);

		r0 = to_executable(buffer).call_i64("l_mul_500");
		CHECK(r0, 500);

		r0 = to_executable(buffer).call_i64("l_umaddl");
		CHECK(r0, 0xF'EB000007);

	};

	TEST (tasml_exec_umulh) {

		std::string code = R"(
			lang aarch64

			section rx
			l_mul_low:
				mov x1, 0xFAFFFFFFFBFFFFFE
				mov x2, 0x0000000100000000
				mul x0, x1, x2
				ret

			l_mul_high:
				mov x1, 0xFAFFFFFFFBFFFFFE
				mov x2, 0x0000000100000000
				umulh x0, x1, x2
				ret
		)";

		tasml::ErrorHandler reporter {vstl_self.name, true};
		SegmentedBuffer buffer = tasml::assemble(vstl_self.name, code);

		uint64_t r0 = to_executable(buffer).call_i64("l_mul_low");
		CHECK(r0, 0xFBFFFFFE00000000);

		r0 = to_executable(buffer).call_i64("l_mul_high");
		CHECK(r0, 0xFAFFFFFF);

	};

	TEST (tasml_exec_udiv) {

		std::string code = R"(
			lang aarch64

			section rx
			l_div:
				mov x1, 507
				mov x2, 12
				udiv x0, x1, x2
				ret
		)";

		tasml::ErrorHandler reporter {vstl_self.name, true};
		SegmentedBuffer buffer = tasml::assemble(vstl_self.name, code);

		uint64_t r0 = to_executable(buffer).call_i64("l_div");
		CHECK(r0, 42);

	};

	TEST (tasml_exec_sdiv) {

		std::string code = R"(
			lang aarch64

			section rx
			l_div:
				mov x1, 507
				mov x2, -12
				sdiv x0, x1, x2
				ret
		)";

		tasml::ErrorHandler reporter {vstl_self.name, true};
		SegmentedBuffer buffer = tasml::assemble(vstl_self.name, code);

		uint64_t r0 = to_executable(buffer).call_i64("l_div");
		CHECK(r0, -42);

	};

	TEST (exec_scall_simple_add) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.label("simple_add");
		writer.put_ldr(X1, X0, 0, Sizing::UX);
		writer.put_ldr(X2, X0, 8, Sizing::UX);
		writer.put_add(X0, X1, X2);
		writer.put_ret();

		ExecutableBuffer buffer = to_executable(segmented);
		CHECK(buffer.scall<int64_t>("simple_add", (int64_t) 11, (int64_t) 133), 144);
		CHECK(buffer.scall<int64_t>("simple_add", (int64_t) 100, (int64_t) 1), 101);
		CHECK(buffer.scall<int64_t>("simple_add", 0x1000'0000'0000'0000, 0x1000'0000'0000'0000), 0x2000'0000'0000'0000);

	};

#endif

}