#pragma once
#include <cstdint>
#include <cstring>
#include <string_view>
#include <string>
#include <vector>

class Strings {

	private:

		std::vector<char> bytes;

	public:

		Strings() {
			bytes.push_back(0);
		}

		uint32_t append(const char* string) {
			return append(std::string_view {string});
		}

		uint32_t append(const std::string& string) {
			return append(std::string_view {string.data(), string.length()});
		}

		uint32_t append(const std::string_view& string) {

			// there is an empty string at the start of all string tables
			if (string.empty()) {
				return 0;
			}

			size_t offset = bytes.size();
			bytes.resize(offset + string.size());
			memcpy(bytes.data() + offset, string.data(), string.size());
			bytes.push_back(0);

			return offset;
		}

		const std::vector<char>& data() const {
			return bytes;
		}

		std::string_view view(uint32_t index) const {
			if (index >= bytes.size()) {
				return "";
			}

			return {bytes.data() + index};
		}

		std::string read(uint32_t index) const {
			return std::string {view(index)};
		}

};
