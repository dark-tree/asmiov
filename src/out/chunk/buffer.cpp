#include "buffer.hpp"

namespace asmio {

	/*
	 * class ChunkBuffer::Appender
	 */

	void ChunkBuffer::Appender::write(uint8_t* data, size_t size) const {
		buffer.reserve(size + buffer.size());
		buffer.insert(buffer.end(), data, data + size);
		*size_ptr += size;
	}

	void ChunkBuffer::Appender::write(uint8_t value, size_t size) const {
		buffer.reserve(size + buffer.size());

		for (size_t i = 0; i < size; i++) {
			buffer.push_back(value);
		}

		*size_ptr += size;
	}

	ChunkBuffer::Appender::Appender(std::vector<uint8_t>& buffer, uint32_t* size_ptr)
		: buffer(buffer), size_ptr(size_ptr) {
	}

	/*
	 * class ChunkBuffer
	 */

	ChunkBuffer::ChunkBuffer(uint32_t align, std::endian endian, ChunkBuffer* root, ChunkBuffer* parent) noexcept
		: alignment(std::max(static_cast<uint32_t>(1), align)), endianness(endian), m_parent(parent), m_root(root) {
	}

	ChunkBuffer::ChunkBuffer(std::endian endian, uint32_t align) noexcept
		: alignment(std::max(static_cast<uint32_t>(1), align)), endianness(endian) {
	}

	ChunkBuffer::ChunkBuffer(uint32_t align, std::endian endian) noexcept
		: alignment(std::max(static_cast<uint32_t>(1), align)), endianness(endian) {
	}


	ChunkBuffer::Appender ChunkBuffer::begin_bytes() {
		if (last_region != ARRAY) {
			m_regions.emplace_back(Array {shared_bytes.size(), 0});
		}

		last_region = ARRAY;
		return {shared_bytes, &std::get<Array>(m_regions.back()).size};
	}

	ChunkBuffer::Ptr ChunkBuffer::begin_chunk(uint32_t align, std::endian endian, const char* name) {
		auto chunk = std::make_shared<ChunkBuffer>(align, endian, m_root, this);
		chunk->name = name;
		last_region = CHUNK;
		m_regions.emplace_back(chunk);
		return chunk;
	}

	void ChunkBuffer::begin_space(uint32_t bytes) {
		if (last_region == ARRAY) {
			push(bytes);
			return;
		}

		if (last_region == SPACE) {
			std::get<Space>(m_regions.back()).size += bytes;
			return;
		}

		last_region = SPACE;
		m_regions.emplace_back(Space {bytes});
	}

	void ChunkBuffer::freeze() {
		this->state = CACHING;

		for (const auto& var : m_regions) {
			if (std::holds_alternative<Ptr>(var)) {
				std::get<Ptr>(var)->freeze();
			}
		}
	}

	void ChunkBuffer::bake(std::vector<uint8_t>& output) const {
		const size_t offset = output.size();
		const size_t padding = util::align_padding(offset, (size_t) alignment);

		output.resize(offset + padding, 0);

		for (const auto& var : m_regions) {

			// we do it region-by-region so that alignment may be calculated
			if (std::holds_alternative<Array>(var)) {
				Array info = std::get<Array>(var);

				auto begin = shared_bytes.begin() + info.offset;
				output.insert(output.end(), begin, begin + info.size);
				continue;
			}

			if (std::holds_alternative<Space>(var)) {
				uint32_t bytes = std::get<Space>(var).size;
				output.resize(output.size() + bytes, 0);
				continue;
			}

			std::get<Ptr>(var)->bake(output);
		}
	}

	void ChunkBuffer::add_link(const Linker& linker) {
		m_root->m_linkers.emplace_back(this, size(), linker);
	}

	void ChunkBuffer::set_root(ChunkBuffer* chunk) {
		this->m_root = chunk;

		for (const auto& var : m_regions) {
			if (std::holds_alternative<Ptr>(var)) {
				std::get<Ptr>(var)->set_root(chunk);
			}
		}
	}

	int ChunkBuffer::index(const ChunkBuffer* child) const {
		int index = 0;

		for (const auto& var : m_regions) {

			// we do it region-by-region so that alignment may be calculated
			if (std::holds_alternative<Ptr>(var)) {
				auto& ptr = std::get<Ptr>(var);

				if (ptr.get() == child) {
					return index;
				}
			}

			index ++;
		}

		throw std::runtime_error {"Unable to calculate index of an out-of-tree chunk!"};
	}

	int ChunkBuffer::index() const {
		if (m_parent == nullptr) {
			return 0;
		}

		return m_parent->index(this);
	}

	size_t ChunkBuffer::regions() const {
		return m_regions.size();
	}

	size_t ChunkBuffer::bytes() const {
		return shared_bytes.size();
	}

	size_t ChunkBuffer::size(size_t offset) const {
		if (state == PRESENT) {
			return m_size;
		}

		size_t total = 0;

		for (const auto& var : m_regions) {

			// we do it region-by-region so that alignment may be calculated
			if (std::holds_alternative<Array>(var)) {
				total += std::get<Array>(var).size;
				continue;
			}

			if (std::holds_alternative<Space>(var)) {
				total += std::get<Space>(var).size;
				continue;
			}

			total += std::get<Ptr>(var)->outer(offset + total);
		}

		if (state == CACHING) {
			m_offset = (int64_t) offset;
			m_size = (int64_t) total;
			state = PRESENT;
		}

		return total;
	}

	size_t ChunkBuffer::outer(size_t unaligned_offset) const {
		size_t padding = util::align_padding(unaligned_offset, static_cast<size_t>(alignment));
		return padding + size(unaligned_offset + padding);
	}

	size_t ChunkBuffer::size() const {
		return size(offset());
	}

	size_t ChunkBuffer::offset() const {
		if (state == PRESENT) {
			return m_offset;
		}

		return m_parent == nullptr ? 0 : util::align_up(m_parent->offset(this), static_cast<size_t>(alignment));
	}

	size_t ChunkBuffer::offset(const ChunkBuffer* child) const {
		size_t offset = this->offset();

		if (child == nullptr) {
			return offset;
		}

		// if the child knows where it is, there is no need to search for it
		if (child->state == PRESENT) {
			return child->m_offset;
		}

		for (const auto& region : m_regions) {

			// we do it region-by-region so that alignment may be calculated
			if (std::holds_alternative<Ptr>(region)) {
				auto& ptr = std::get<Ptr>(region);

				if (ptr.get() == child) {
					return offset;
				}

				offset += ptr->outer(offset);
				continue;
			}

			if (std::holds_alternative<Space>(region)) {
				offset += std::get<Space>(region).size;
				continue;
			}

			offset += std::get<Array>(region).size;
		}

		throw std::runtime_error {"Unable to calculate offset of an out-of-tree chunk!"};
	}

	ChunkBuffer* ChunkBuffer::root() {
		return m_root;
	}

	ChunkBuffer* ChunkBuffer::adopt(const Ptr& orphan) {

		if (orphan->m_parent != nullptr) {
			throw std::runtime_error {"Unable to adopt element from another tree!"};
		}

		orphan->m_parent = this;
		orphan->set_root(m_root);

		for (Link& link : orphan->m_linkers) {
			m_root->m_linkers.emplace_back(link);
		}

		orphan->m_linkers.clear();
		orphan->m_linkers.shrink_to_fit();

		last_region = CHUNK;
		m_regions.emplace_back(orphan);
		return this;
	}

	ChunkBuffer* ChunkBuffer::merge(const Ptr& orphan) {

		if (orphan->m_parent != nullptr) {
			throw std::runtime_error {"Unable to adopt element from another tree!"};
		}

		std::vector<uint8_t> buffer;
		buffer.reserve(orphan->bytes());

		orphan->bake(buffer);
		this->write(buffer);

		return this;
	}

	ChunkBuffer* ChunkBuffer::clear() {
		if (has_links) {
			throw std::runtime_error {"Unable to clear a buffer with links!"};
		}

		for (const auto& region : m_regions) {
			if (std::holds_alternative<Ptr>(region)) {
				throw std::runtime_error {"Unable to clear a buffer with childrens!"};
			}
		}

		m_regions.clear();
		shared_bytes.clear();
		last_region = UNSET;

		return this;
	}

	ChunkBuffer::Ptr ChunkBuffer::chunk(const char* name) {
		return begin_chunk(1, endianness, name);
	}

	ChunkBuffer::Ptr ChunkBuffer::chunk(uint32_t align, const char* name) {
		return begin_chunk(align, endianness, name);
	}

	ChunkBuffer::Ptr ChunkBuffer::chunk(std::endian endian, uint32_t align, const char* name) {
		return begin_chunk(align, endian, name);
	}

	std::vector<uint8_t> ChunkBuffer::bake() {
		std::vector<uint8_t> buffer;
		freeze();
		bake(buffer);

		for (auto& link : m_linkers) {
			size_t offset = link.target->offset() + link.offset;
			link.linker(buffer.data() + offset);
		}

		return buffer;
	}

}
