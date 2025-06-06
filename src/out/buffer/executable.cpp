
#include "executable.hpp"

namespace asmio {

	/*
	 * ExecutableBuffer
	 */

	ExecutableBuffer::ExecutableBuffer(size_t total) {

		const size_t page = getpagesize();

		// this value should already be page aligned, but let's check anyway
		length = ALIGN_UP(total, page);

		// create a basic memory map, after this we will set the correct flags for each segment
		buffer = (uint8_t*) mmap(nullptr, length, PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

		if (buffer == nullptr) {
			throw std::runtime_error {"Failed to allocate memory map!"};
		}

	}

	ExecutableBuffer::~ExecutableBuffer() {
		munmap(buffer, length);
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
			mprotect(data, segment.size(), segment.get_mprot_flags());
		}

		// copy the label map
		labels = segmented.resolved_labels();

	}

	uint8_t* ExecutableBuffer::address() const {
		return buffer;
	}

	uint8_t* ExecutableBuffer::address(Label label) const {
		return buffer + labels.at(label);
	}

	size_t ExecutableBuffer::size() const {
		return length;
	}

	/*
	 * Functions
	 */

	ExecutableBuffer to_executable(SegmentedBuffer& segmented) {

		const size_t page = getpagesize();
		segmented.align(page);

		// after alignment we know how big the buffer needs to be
		ExecutableBuffer buffer {segmented.total()};

		// now that we have a buffer allocated we can link
		segmented.link((uint64_t) buffer.address());

		// finally copy data and setting to the final image
		buffer.bake(segmented);

		return buffer;
	}

}
