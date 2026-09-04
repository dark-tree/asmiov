
#include "executable.hpp"

#include <asmio/util/platform.hpp>
#include <asmio/util.hpp>

namespace asmio {

	/*
	 * ExecutableBuffer
	 */

	ExecutableBuffer::ExecutableBuffer(size_t total) {

		const size_t page = page_size();

		// this value should already be page aligned, but let's check anyway
		length = util::align_up(total, page);

		// create a basic memory map, after this we will set the correct flags for each segment
		buffer = (uint8_t*) allocate_pages(length, MemoryFlag::W);

		if (buffer == nullptr) {
			throw std::runtime_error {"Failed to allocate memory map!"};
		}

	}

	ExecutableBuffer::ExecutableBuffer(uint8_t* buffer, size_t length) noexcept
		: buffer(buffer), length(length) {
	}

	ExecutableBuffer::ExecutableBuffer(ExecutableBuffer&& other) noexcept {
		labels = std::move(other.labels);

		std::swap(buffer, other.buffer);
		std::swap(length, other.length);
	}

	ExecutableBuffer::ExecutableBuffer(const ExecutableBuffer& other) {
		labels = other.labels;
		buffer = (uint8_t*) allocate_pages(other.length, MemoryFlag::W);
		length = other.length;
	}

	ExecutableBuffer::~ExecutableBuffer() {
		if (buffer != nullptr) {
			free_pages(buffer, length);
		}

		buffer = nullptr;
		length = 0;
	}

	ExecutableBuffer& ExecutableBuffer::operator=(ExecutableBuffer&& other) noexcept {
		labels = std::move(other.labels);

		std::swap(buffer, other.buffer);
		std::swap(length, other.length);
		return *this;
	}

	void ExecutableBuffer::bake(SegmentedBuffer& segmented) {

		if (segmented.total() != length) {
			throw std::runtime_error {"Invalid buffer size!"};
		}

		// initialize pages
		for (const BufferSegment& segment : segmented.segments()) {

			uint8_t* data = buffer + segment.start;
			size_t bytes = segment.buffer.size();

			if (bytes == 0) {
				continue;
			}

			memcpy(data, segment.buffer.data(), bytes);
			memset(data + bytes, segment.padder, segment.tail);
			protect_pages(data, segment.size(), segment.flags);
		}

		// copy the label map
		labels = segmented.resolved_labels();

	}

	uint8_t* ExecutableBuffer::address() const {
		return buffer;
	}

	uint8_t* ExecutableBuffer::own() {
		uint8_t* ptr = buffer;
		buffer = nullptr;
		return ptr;
	}

	uint8_t* ExecutableBuffer::address(const Label& label) const {
		return buffer + offset(label);
	}

	uint64_t ExecutableBuffer::offset(const Label& label) const {
		return labels.at(label);
	}

	size_t ExecutableBuffer::size() const {
		return length;
	}

	/*
	 * Functions
	 */

	ExecutableBuffer to_executable(SegmentedBuffer& segmented) {

		const size_t page = page_size();
		segmented.align(page);

		// after alignment, we know how big the buffer needs to be
		ExecutableBuffer buffer {segmented.total()};

		// now that we have a buffer allocated we can link
		segmented.link(reinterpret_cast<uint64_t>(buffer.address()));

		// finally copy data and setting to the final image
		buffer.bake(segmented);

		return buffer;
	}

}
