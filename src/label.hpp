#pragma once

#include <cstring>
#include "util.hpp"
#include "const.hpp"

struct Label {

	private:

		const char* str;
		uint64_t hash;

	public:

		Label(const char* str)
		: str(str), hash(hash_djb2(str)) {}

		bool operator == (Label label) const {
			return label.hash == this->hash && strcmp(label.str, this->str) == 0;
		}

		struct HashFunction {

			size_t operator() (const Label& label) const {
				return label.hash;
			}

		};

};

struct LabelCommand {

	Label label;
	uint8_t size;
	uint32_t offset;

};