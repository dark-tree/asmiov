
#include "segmented.hpp"

namespace asmio {

	/*
	 * BufferSegment
	 */

	BufferSegment::BufferSegment(uint32_t index, uint8_t flags) noexcept
		: index(index), flags(flags) {
	}

	size_t BufferSegment::size() const {
		return buffer.size() + tail;
	}

	bool BufferSegment::empty() const {
		return buffer.empty();
	}

	BufferMarker BufferSegment::current() const {
		return {index, static_cast<uint32_t>(buffer.size())};
	}

	size_t BufferSegment::align(size_t start, size_t page) {
		this->start = start;
		const int64_t bytes = buffer.size();
		const int64_t aligned = ALIGN_UP(bytes, page);

		// extra bytes to pad to page boundary
		this->tail = aligned - bytes;
		return aligned;
	}

	int BufferSegment::get_mprot_flags() const {
		// FIXME this is dumb

		int protect = 0;
		if (flags & R) protect |= PROT_READ;
		if (flags & W) protect |= PROT_WRITE;
		if (flags & X) protect |= PROT_EXEC;
		return protect;
	}

	/*
	 * SegmentedBuffer
	 */

	SegmentedBuffer::SegmentedBuffer() {
		use_section(BufferSegment::DEFAULT);
	}

	BufferMarker SegmentedBuffer::current() const {
		return sections[selected].current();
	}

	int64_t SegmentedBuffer::get_offset(BufferMarker marker) const {
		return sections.at(marker.section).start + marker.offset;
	}

	uint8_t* SegmentedBuffer::get_pointer(BufferMarker marker) {
		return sections.at(marker.section).buffer.data() + marker.offset;
	}

	void SegmentedBuffer::align(size_t page) {
		size_t offset = 0;

		// align sections to page boundaries
		for (BufferSegment& segment : sections) {
			offset = segment.align(offset, page);
		}
	}

	void SegmentedBuffer::link(size_t base, const Linkage::Handler& handler) {
		base_address = base;

		for (const Linkage& linkage : linkages) {
			try {
				linkage.linker(this, linkage, base);
			} catch (std::runtime_error& error) {
				if (handler) handler(linkage, error.what()); else throw;
			}
		}
	}

	void SegmentedBuffer::add_linkage(const Label& label, int shift, const Linkage::Linker& linker) {
		uint32_t offset = sections[selected].buffer.size();
		linkages.emplace_back(label, BufferMarker {(uint32_t) selected, offset + shift}, linker);
	}

	BufferMarker SegmentedBuffer::get_label(const Label& label) {
		auto it = labels.find(label);

		if (it != labels.end()) {
			return it->second;
		}

		throw std::runtime_error {"Undefined label '" + std::string(label.c_str()) + "' used"};
	}

	void SegmentedBuffer::add_label(const Label& label) {
		auto it = labels.find(label);

		if (it == labels.end()) {
			labels[label] = sections[selected].current();
			return;
		}

		throw std::runtime_error {"Can't redefine label '" + std::string(label.c_str()) + "', in section #" + std::to_string(selected)};
	}

	bool SegmentedBuffer::has_label(const Label& label) {
		return labels.contains(label);
	}

	void SegmentedBuffer::push(uint8_t byte) {
		auto& buffer = sections[selected].buffer;
		buffer.push_back(byte);
	}

	void SegmentedBuffer::fill(int64_t bytes, uint8_t value) {
		auto& buffer = sections[selected].buffer;
		buffer.resize(buffer.size() + bytes, value);
	}

	void SegmentedBuffer::insert(uint8_t* data, size_t bytes) {
		auto& buffer = sections[selected].buffer;
		buffer.insert(buffer.end(), data, data + bytes);
	}

	void SegmentedBuffer::use_section(uint8_t flags) {
		int index = -1;
		const int count = sections.size();

		for (int i = 0; i < count; ++i) {
			if (sections[i].flags == flags) {
				index = i;
			}
		}

		// section already exists
		if (index != -1) {
			selected = index;
			return;
		}

		// create new section
		selected = count;
		sections.emplace_back(count, flags);
	}

	size_t SegmentedBuffer::count() const {
		return sections.size();
	}

	size_t SegmentedBuffer::total() {
		auto last = sections.back();
		return last.start + last.buffer.size() + last.tail;
	}

	void SegmentedBuffer::dump() const {
		std::cout << "./unasm.sh " << base_address << " \"";

		for (const BufferSegment& segment : sections) {
			std::cout << "SECTION " << (segment.flags & BufferSegment::X ? ".text" : ".data") << " \\ndb ";
			bool first = true;

			for (uint8_t byte : segment.buffer) {
				if (!first) {
					std::cout << ", ";
				}

				first = false;
				std::cout << '0' << std::setfill('0') << std::setw(2) << std::hex << ((int) byte) << "h";
			}

			std::cout << "\\n";
		}

		std::cout << "\\n\"" << std::endl;
	}

	const std::vector<BufferSegment>& SegmentedBuffer::segments() const {
		return sections;
	}

	LabelMap<size_t> SegmentedBuffer::resolved_labels() const {
		LabelMap<size_t> result;

		for (auto& [label, marker] : labels) {
			result[label] = get_offset(marker);
		}

		return result;
	}

}