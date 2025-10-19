#pragma once

#include "asm/aarch64/writer.hpp"

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
			writer.put_inst_movz(W(0), 0x102A, 15);
		};

		EXPECT_THROW(std::runtime_error) {
			writer.put_inst_movz(W(0), 0x102A, 32);
		};

		EXPECT_THROW(std::runtime_error) {
			writer.put_inst_movz(X(0), 0x102A, 64);
		};

		EXPECT_THROW(std::runtime_error) {
			writer.put_inst_movz(SP, 0);
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
			writer.put_inst_orr(X(0), X(1), 0b10'000000'110000);
		};

		EXPECT_THROW(std::runtime_error) {
			writer.put_inst_orr(X(0), X(1), 0);
		};

		EXPECT_THROW(std::runtime_error) {
			writer.put_inst_orr(X(0), X(1), std::numeric_limits<uint64_t>::max());
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

		writer.put_inst_movz(X(0), 1, 0); // this should be overridden
		writer.put_inst_movz(X(0), 0x102A, 16);
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 0x102A'0000)

	};

	TEST (writer_exec_movk) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_inst_movz(X(0), 1, 0); // this should be overridden
		writer.put_inst_movk(X(0), 0x102A, 16);
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 0x102A'0001)

	};

	TEST (writer_exec_movn) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_inst_movn(X(0), 0x00FF, 16);
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 0xFFFF'FFFF'FF00'FFFF);

	};

	TEST (writer_exec_orr) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_inst_movz(X(4), 0x0021);
		writer.put_inst_movz(X(3), 0x4300);
		writer.put_inst_orr(X(2), X(3), X(4));
		writer.put_inst_orr(X(0), XZR, X(2));
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 0x4321);

	};

	TEST (writer_exec_orr_bitmask) {

		SegmentedBuffer segmented;
		BufferWriter writer {segmented};

		writer.put_inst_movz(X(1), 0b00000101);
		writer.put_inst_orr(X(0), X(1), 0b1001100110011001100110011001100110011001100110011001100110011001);
		writer.put_ret();

		uint64_t r0 = to_executable(segmented).call_u64();
		CHECK(r0, 0b1001100110011001100110011001100110011001100110011001100110011101);

	};

#endif

}