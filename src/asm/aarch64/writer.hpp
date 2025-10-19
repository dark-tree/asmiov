#pragma once
#include <out/buffer/writer.hpp>

#include "argument/registry.hpp"
#include "argument/shift.hpp"

namespace asmio::arm {

	class BufferWriter : public BasicBufferWriter {

		public:

			/**
			 * Try to compute the ARM immediate bitmask based on a target value, this is a multi-step process
			 * that tries to guess the correct coefficients, if that fails throws an std::runtime_error.
			 *
			 * @param value The value to encode
			 * @param wide If the bitmask is expected to be used in a "wide" (64 bit) context
			 *
			 * @return Combined 'N:immr:imms' bit pattern
			 */
			uint16_t compute_immediate_bitmask(uint64_t value, bool wide) {

				if (!wide) {

					// this is a interesting case - a 64 bit immediate pattern used for 32 bit operation, we could just allow it, for not throw
					if (value > 0xFFFFFFFF) throw std::runtime_error {"Invalid pattern, the value is not encodable!"};

					// copy the lower bits up to maintain tha pattern across the whole 64 bit number
					value |= value << 32;
				}

				if (value == 0 || value == std::numeric_limits<uint64_t>::max()) {
					throw std::runtime_error {"Invalid pattern, the value is not encodable!"};
				}

				for (uint64_t size : {2, 4, 8, 16, 32, 64}) {

					if (size == 64 && !wide) break;

					// mask all bits smaller or equal the one selected
					const uint64_t mask = size != 64
						? (1 << size) - 1
						: std::numeric_limits<uint64_t>::max();

					// the first element, we compare all others against this one
					// for 64 bit values this will just be used as-is
					const uint64_t pattern = value & mask;

					// the whole value would need to be zero,
					// this is just a simple short path for simple cases
					if (pattern == 0) {
						continue;
					}

					uint64_t source = value;
					bool matched = true;

					for (size_t i = size; i < 64; i += size) {
						source >>= size;

						// if any element doesn't match this is not the valid element size
						// or the value is not encodable - but we can't tell that right now
						if ((source & mask) != pattern) {
							matched = false;
							break;
						}
					}

					if (matched) {

						// we now know the size, but can't yet tell if the actual pattern is encodable
						// nor the correct rotation that needs to be applied, delegate that to the next function
						return compute_element_bitmask(value, size);
					}
				}

				// realistically this can only happen for non-wide numbers
				throw std::runtime_error {"Invalid pattern, the value is not encodable!"};
			}

			/**
			 * Try to compute the ARM immediate bitmask based on a target value, this is a multi-step process
			 * that tries to guess the correct coefficients, if that fails throws an std::runtime_error.
			 *
			 * @param value The bit pattern to try encoding
			 * @param size Size of one repeating element within the pattern
			 *
			 * @return Combined 'N:immr:imms' bit pattern
			 */
			uint16_t compute_element_bitmask(uint64_t value, int size) {

				const uint64_t mask = (1 << size) - 1;
				const uint64_t pattern = value & mask;
				const int bits = std::popcount(pattern);

				// shift register
				uint64_t shift = value;

				for (int i = 0; i < size; i ++) {

					// ctz(~x) == cto(x), if all bits are trailing that means the current
					// rotation positioned all of them at the end, creating a valid pattern
					if (__builtin_ctz(~shift) == bits) {
						return pack_bitmask(size, bits, i);
					}

					shift = std::rotl(shift, 1);
				}

				throw std::runtime_error {"Invalid pattern, the value is not encodable!"};
			}

			/**
			 * Constructs the N:immr:imms value for bitmask immediate instructions
			 *
			 * @param size size of one element in bits (2,4,8,16,32,64)
			 * @param ones number of ones in the element (1 <= ones <= size)
			 * @param roll right-roll of the elements within one pattern element (roll < size)
			 *
			 * @return Combined 'N:immr:imms' bit pattern
			 */
			uint16_t pack_bitmask(uint32_t size, uint32_t ones, uint32_t roll) {
				if (ones >= size || ones == 0) throw std::runtime_error {"Invalid bitmask, run-of-ones must be grater than 0 and smaller than the element size."};
				if (roll >= size) throw std::runtime_error {"Invalid bitmask, rotation must be smaller then element size."};
				if (std::popcount(size) != 1 || size > 64 || size < 2) throw std::runtime_error {"Invalid bitmask, size must be one of (2, 4, 8, 16, 32, 64)."};

				// N | imms        | size    | run-of-ones
				// - + ----------- + ------- + -----------
				// 0 | 1 1 1 1 0 x | 2 bits  | 1
				// 0 | 1 1 1 0 x x | 4 bits  | 1-3
				// 0 | 1 1 0 x x x | 8 bits  | 1-7
				// 0 | 1 0 x x x x | 16 bits | 1-15
				// 0 | 0 x x x x x | 32 bits | 1-31
				// 1 | x x x x x x | 64 bits | 1-63
				uint32_t nimms = 0b0'111111;

				// clears the '0' bit separating 'size' from 'run-of-ones'
				// this will ALSO set the N to 1 for 64 bit numbers, as that is the only non-1 bit
				nimms ^= size;

				// next we clear all the 'x' bits (set them to 0) by
				// ANDing them with the inverse of the mask
				nimms &= ~(size - 1);

				// at this point we can encode the run-of-ones value by ORing it with the nimms
				// do note that the encoded value is decremented by one, so run length 1 is saved as 0, all ones length is not valid
				nimms |= (ones - 1);

				// as the N bit is saved separately we have to extract it back from our value
				const uint32_t n = (nimms & 0b1'000000) >> 6;

				// same here, we mask out the N from nimms to get the imms part
				return n << 12 | roll << 6 | (nimms & 0b0'111111);
			}

			uint8_t pack_shift(uint8_t shift, bool wide) {
				if (shift & 0b0000'1111) throw std::runtime_error {"Invalid shift, only multiples of 16 allowed"};
				if (shift & 0b1100'0000) throw std::runtime_error {"Invalid shift, the maximum value of 48 exceeded"};

				const uint8_t hw = shift >> 4;
				if (!wide && hw > 1) throw std::runtime_error {"Invalid shift, only 0 or 16 bit shifts allowed in 32 bit context"};

				return hw;
			}

			void assert_register_triplet(Registry a, Registry b, Registry c) {
				if (a.wide() != b.wide() || a.wide() != c.wide()) {
					throw std::runtime_error {"Invalid operands, all given registers need to be of the same width."};
				}
			}

			void put_inst_mov_op(Registry registry, uint16_t opc, uint16_t imm, uint16_t shift) {
				uint16_t sf = registry.wide() ? 1 : 0;
				uint16_t hw = pack_shift(shift, registry.wide());

				if (!registry.is(Registry::GENERAL)) {
					throw std::runtime_error {"Invalid operand, expected general purpose register."};
				}

				put_dword(sf << 31 | opc << 23 | hw << 21 | imm << 5 | registry.reg);
			}

			void put_inst_movz(Registry registry, uint16_t imm, uint16_t shift = 0) {
				put_inst_mov_op(registry, 0b10100101, imm, shift);
			}

			void put_inst_movk(Registry registry, uint16_t imm, uint16_t shift = 0) {
				put_inst_mov_op(registry, 0b11100101, imm, shift);
			}

			void put_inst_movn(Registry registry, uint16_t imm, uint16_t shift = 0) {
				put_inst_mov_op(registry, 0b00100101, imm, shift);
			}

			void put_inst_orr(Registry destination, Registry a, Registry b, ShiftType shift = ShiftType::LSL, uint8_t imm6 = 0) {
				assert_register_triplet(a, b, destination);

				// if any one of them is a stack register abort
				if (!a.is(Registry::GENERAL) || !b.is(Registry::GENERAL) || !destination.is(Registry::GENERAL)) {
					throw std::runtime_error {"Invalid operands, expected general purpose registers."};
				}

				uint16_t sf = destination.wide() ? 1 : 0;
				put_dword(sf << 31 | 0b0101010 << 24 | uint8_t(shift) << 22 | a.reg << 16 | imm6 << 10 | b.reg << 5 | destination.reg);
			}

			void put_inst_orr(Registry destination, Registry source, uint64_t pattern) {

				// destination can be SP
				if (!source.is(Registry::GENERAL)) {
					throw std::runtime_error {"Invalid operand, expected source to be a general purpose register."};
				}

				if (destination.wide() != source.wide()) {
					throw std::runtime_error {"Invalid operands, all given registers need to be of the same width."};
				}

				uint16_t sf = destination.wide() ? 1 : 0;
				put_dword(sf << 31 | 0b01100100 << 23 | compute_immediate_bitmask(pattern, destination.wide()) << 10 | source.reg << 5 | destination.reg);
			}

		public:

			BufferWriter(SegmentedBuffer& buffer);

			INST put_nop();
			INST put_ret();
			INST put_ret(Registry registry);

	};

}