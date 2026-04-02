#pragma once
#include <cstdint>
#include <cstdlib>
#include <algorithm>

template <typename T>
class Block {

	private:

		T* m_elements = nullptr;
		Block* m_previous = nullptr;
		Block* m_next = nullptr;
		uint32_t m_capacity = 0;
		uint32_t m_size = 0;

	public:

		Block(uint32_t capacity) {
			this->m_elements = new T[capacity];
			this->m_capacity = capacity;
		}

		~Block() {
			if (m_previous) {
				delete m_previous;
				m_previous = nullptr;
			}

			if (m_elements) {
				delete[] m_elements;
				m_elements = nullptr;
			}

			this->m_capacity = 0;
			this->m_size = 0;
		}

		Block(Block&& other) noexcept {
			this->m_elements = other.m_elements;
			this->m_previous = other.m_previous;
			this->m_capacity = other.m_capacity;
			this->m_size = other.m_size;

			this->m_elements = nullptr;
			this->m_previous = nullptr;
		}

		Block(Block* previous)
			: Block(previous->m_capacity * 2) {
			m_previous = previous;
			previous->m_next = this;
		}

		Block() = default;
		Block(const Block& other) = delete;

	public:

		class Iterator {

			private:

				Block* block = nullptr;
				uint32_t index = 0;

			public:

				Iterator(Block* block, uint32_t index)
					: block(block), index(index) {
				}

				T* operator*() {
					return block->get(index);
				}

				Iterator& operator++() {
					index ++;

					if (index >= block->m_size) {
						block = block->m_next;
						index = 0;
					}

					return *this;
				}

				bool operator!=(const Iterator& other) const {
					if (block == other.block) {
						return index != other.index;
					}

					return true;
				}

		};

	public:

		T* push(const T& value) {
			if (m_size >= m_capacity) {
				return nullptr;
			}

			T* slot = m_elements + m_size;
			*slot = value;
			m_size ++;
			return slot;
		}

		T* get(uint32_t index) {
			return m_elements + index;
		}

		const T* get(uint32_t index) const {
			return m_elements + index;
		}

		uint32_t size() const {
			return m_size + (m_previous ? m_previous->size() : 0);
		}

		uint32_t count() const {
			return 1 + (m_previous ? m_previous->count() : 0);
		}

		Block* first() {
			return m_previous ? m_previous->first() : this;
		}

		bool empty() const {
			return m_size == 0;
		}

		Iterator begin() {
			return {this, 0};
		}

		Iterator end() {
			return {nullptr, 0};
		}

};

template <typename T>
class Pool {

	private:

		Block<T>* block = nullptr;

	public:

		Pool(int initial_capacity) {
			block = new Block<T>(initial_capacity);
		}

		~Pool() {
			if (block) {
				delete block;
				block = nullptr;
			}
		}

		Pool(Pool&& other) noexcept
			: Pool() {
			std::swap(block, other.block);
		}

		Pool() = default;
		Pool(const Pool& other) = delete;

	public:

		T* push(const T& value) {
			T* ptr = block->push(value);

			// append new block and retry
			if (!ptr) {
				block = new Block<T>(block);
				ptr = block->push(value);
			}

			return ptr;
		}

		uint32_t size() const {
			return block == nullptr ? 0 : block->size();
		}

		bool empty() const {
			return size() == 0;
		}

		uint32_t blocks() const {
			return block == nullptr ? 0 : block->count();
		}

		Block<T>::Iterator begin() const {
			if (!block) return end();
			if (block->empty()) return end();

			return {block->first(), 0U};
		}

		Block<T>::Iterator end() const {
			return {nullptr, 0U};
		}

};