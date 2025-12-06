#include "pattern.hpp"

#include <util.hpp>

namespace asmio::arm {

	/*
	 * class BitPattern
	 */

	std::optional<uint16_t> BitPattern::compute_immediate_bitmask(uint64_t value) {

		if (value == 0 || value == std::numeric_limits<uint64_t>::max()) {
			return std::nullopt;
		}

		for (uint64_t size : {2, 4, 8, 16, 32, 64}) {

			// mask all bits smaller or equal the one selected
			const auto mask = util::bit_fill<uint64_t>(size);

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
		return std::nullopt;
	}

	std::optional<uint16_t> BitPattern::compute_element_bitmask(uint64_t value, size_t size) {

		const auto mask = util::bit_fill<uint64_t>(size);
		const uint64_t pattern = value & mask;
		const int bits = std::popcount(pattern);

		// shift register
		uint64_t shift = value;

		for (size_t i = 0; i < size; i ++) {

			// if all bits are trailing that means the current
			// rotation positioned all of them at the end, creating a valid pattern
			if (util::count_trailing_ones(shift) == bits) {
				return pack_bitmask(size, bits, i);
			}

			shift = std::rotl(shift, 1);
		}

		return std::nullopt;
	}

	uint16_t BitPattern::pack_bitmask(uint32_t size, uint32_t ones, uint32_t roll) {

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

	BitPattern BitPattern::try_pack(uint64_t immediate) {
		return {compute_immediate_bitmask(immediate)};
	}

	BitPattern::BitPattern(uint64_t immediate) {
		auto value = compute_immediate_bitmask(immediate);

		if (value) {
			m_bitmask = value.value();
			m_valid = true;
			return;
		}

		throw std::runtime_error {"Invalid bit pattern, unable to encode"};
	}

	BitPattern::BitPattern(uint32_t size, uint32_t length, uint32_t roll) {
		if NOT (size >= 1 && size <= 64 && std::popcount(size) == 1) {
			throw std::runtime_error {"Invalid bit pattern, size (" + std::to_string(size) + ") is not one of 2, 4, 8, 16, 32, 64"};
		}

		if NOT (1 <= length && length < size) {
			throw std::runtime_error {"Invalid bit pattern, length (" + std::to_string(length) + ") must be between 1 and " + std::to_string(size - 1)};
		}

		if NOT (roll < size) {
			throw std::runtime_error {"Invalid bit pattern, roll (" + std::to_string(roll) + ") must be between 0 and " + std::to_string(size - 1)};
		}

		m_bitmask = pack_bitmask(size, length, roll);
		m_valid = true;
	}

	bool BitPattern::ok() const {
		return m_valid;
	}

	bool BitPattern::wide() const {
		return m_valid && (m_bitmask & 0b1'000000'000000);
	}

	uint32_t BitPattern::bitmask() const {
		if (m_valid) {
			return m_bitmask;
		}

		throw std::runtime_error {"Invalid bit pattern used as operand"};
	}

}
