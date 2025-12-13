#include "codecs.hpp"

namespace asmio {

	/*
	 * class UnsignedLeb128
	 */

	void UnsignedLeb128::encode(ChunkBuffer& buffer, uint64_t value) {
		while (true) {
			const uint8_t part = value & 0x7F;

			if (value >>= 7) {
				buffer.put<uint8_t>(part | 0x80);
				continue;
			}

			buffer.put<uint8_t>(part);
			break;
		}
	}

	/*
	 * class SignedLeb128
	 */

	void SignedLeb128::encode(ChunkBuffer& buffer, int64_t signed_value) {
		auto value = std::bit_cast<uint64_t>(signed_value);

		const auto minus_one = std::bit_cast<uint64_t>(-1L);
		const bool negative = signed_value < 0;
		bool next = true;

		while (next) {
			uint8_t byte = value & 0x7F;
			value >>= 7;

			// this is only neccecary if the implementation of >>=
			// uses a logical shift, in practice most C++ implementations
			// would (hopefully) use a arythmetic shift when the shifted value is
			// of a signed type, however, this behaviour is not well defined and
			// relies on undefined behaviour. To avoit it, we explicitly use a
			// non-singed type here and implements arythmetic shift ourselves.
			if (negative) {
				value |= ~0UL << (sizeof(signed_value) * 8 - 7); // sign extend
			}

			// sign bit of byte is second high-order bit
			const uint8_t sign_bit = byte & 0x40;

			if ((value == 0 && sign_bit == 0) || (value == minus_one && sign_bit != 0)) {
				next = false;
			} else {
				byte |= 0x80;
			}

			buffer.put<uint8_t>(byte);
		}
	}

	void DynamicInt::encode(ChunkBuffer& buffer, std::tuple<int, uint64_t> pack) {
		int bytes = std::get<0>(pack);
		uint64_t value = std::get<1>(pack);

		if (bytes == 1) {
			buffer.put<uint8_t>(value);
			return;
		}

		if (bytes == 2) {
			buffer.put<uint16_t>(value);
			return;
		}

		if (bytes == 4) {
			buffer.put<uint32_t>(value);
			return;
		}

		if (bytes == 8) {
			buffer.put<uint64_t>(value);
			return;
		}

		throw std::runtime_error("Invalid byte width in DynamicInt codec!");
	}

}
