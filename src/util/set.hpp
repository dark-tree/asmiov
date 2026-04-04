#pragma once

#include "external.hpp"

namespace asmio::util {

	template <typename T>
	class IndexedSet {

		private:

			size_t next = 0;
			std::unordered_map<T, size_t> indecies;
			std::vector<T> values;

		public:

			size_t put(const T& value) {
				auto it = indecies.find(value);

				if (it == indecies.end()) {
					indecies[value] = next;
					values.push_back(value);
					return next ++;
				}

				return it->second;
			}

			const T& get(size_t index) const {
				if (index < values.size()) {
					return values[index];
				}

				throw std::out_of_range("index out of range");
			}

			size_t index(const T& value) const {
				auto it = indecies.find(value);

				if (it == indecies.end()) {
					throw std::out_of_range("no such element");
				}

				return it->second;
			}

			const std::vector<T>& items() const {
				return values;
			}

	};

}