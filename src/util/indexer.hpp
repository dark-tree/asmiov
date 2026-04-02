#pragma once
#include <concepts>

template <std::integral T, T initial = 0>
class Indexer {

	private:

		T counter = initial;

	public:

		T next() {
			return counter ++;
		}

		T peek() const {
			return counter;
		}

};
