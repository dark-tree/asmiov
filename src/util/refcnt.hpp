#pragma once

#include "external.hpp"

namespace asmio {

	using RefHeader = size_t;

	template <typename T>
	RefHeader& ref_count(T* buffer) {
		auto* byte_buffer = reinterpret_cast<uint8_t*>(buffer);
		return *reinterpret_cast<RefHeader*>(byte_buffer - sizeof(RefHeader));
	}

	template <typename T> [[nodiscard]]
	T* ref_allocate(size_t count) {
		auto* byte_buffer = static_cast<uint8_t*>(malloc(count * sizeof(T) + sizeof(RefHeader)));
		*reinterpret_cast<RefHeader*>(byte_buffer) = 1;

		return reinterpret_cast<T*>(byte_buffer + sizeof(RefHeader));
	}

	template <typename T>
	bool ref_free(T* buffer) {
		if (buffer == nullptr) {
			return false;
		}

		auto& count = ref_count(buffer);

		if (0 == -- count) {
			free(&count);
			return true;
		}

		return false;
	}

	template <typename T>
	void ref_increment(T* buffer) {
		if (buffer == nullptr) {
			return;
		}

		ref_count(buffer) += 1;
	}

}