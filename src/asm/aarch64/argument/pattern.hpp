#pragma once

#include <util.hpp>
#include <out/buffer/sizes.hpp>

#include "external.hpp"

namespace asmio::arm {

	class  BitPattern {

		private:

			uint16_t m_bitmask = 0;
			bool m_valid = false;

			BitPattern(std::optional<uint16_t> pattern) {
				if (pattern) {
					m_bitmask = pattern.value();
				}

				m_valid = pattern.has_value();
			}

			/**
			 * Try to compute the ARM immediate bitmask based on a target value, this is a multi-step process
			 * that tries to guess the correct coefficients, if that fails it returns a std::nullopt.
			 *
			 * @param value The value to encode
			 *
			 * @return Combined 'N:immr:imms' bit pattern, or nullopt.
			 */
			static std::optional<uint16_t> compute_immediate_bitmask(uint64_t value);

			/**
			 * Try to compute the ARM immediate bitmask based on a target value, this is a multi-step process
			 * that tries to guess the correct coefficients, if that fails it returns a std::nullopt.
			 *
			 * @param value The bit pattern to try encoding
			 * @param size Size of one repeating element within the pattern
			 *
			 * @return Combined 'N:immr:imms' bit pattern, or nullopt.
			 */
			static std::optional<uint16_t> compute_element_bitmask(uint64_t value, size_t size);

			/**
			 * Constructs the N:immr:imms value for bitmask immediate instructions,
			 * the correctness of provided arguments IS NOT CHECKED.
			 *
			 * @param size size of one element in bits (2,4,8,16,32,64)
			 * @param ones number of ones in the element (1 <= ones <= size)
			 * @param roll right-roll of the elements within one pattern element (roll < size)
			 *
			 * @return Combined 'N:immr:imms' bit pattern
			 */
			static uint16_t pack_bitmask(uint32_t size, uint32_t ones, uint32_t roll);

		public:

			/// Try to create bit pattern based on immediate pattern or create invalid pattern otherwise
			static BitPattern try_pack(uint64_t immediate);

			/// Create bit pattern based on immediate pattern or throw
			BitPattern(uint64_t immediate);

			/// Create bit pattern based on explicit coefficients or throw
			BitPattern(uint32_t size, uint32_t length, uint32_t roll);

		public:

			/// Check if this bit pattern exists
			bool ok() const;

			/// Check if this bit pattern encodes an explicitly 64 bit value
			bool wide() const;

			/// Get the encoded bitmask, or throw
			uint32_t bitmask() const;

	};

}