
#include "segmented.hpp"

#include <utility>

namespace asmio {

	/*
	 * class BufferSegment
	 */

	BufferSegment::BufferSegment(uint32_t index, MemoryFlags flags, std::string name) noexcept
		: index(index), flags(flags), name(std::move(name)) {
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

	size_t BufferSegment::align(size_t offset, size_t page) {
		this->start = offset;
		const size_t bytes = buffer.size();
		const size_t aligned = util::align_up(bytes, page);

		// extra bytes to pad to page boundary
		this->tail = aligned - bytes;
		return offset + aligned;
	}

	const char* BufferSegment::default_name(MemoryFlags flags) {

		// normal sections
		if (flags == MemoryFlag::R) return ".rodata";
		if (flags == (MemoryFlag::R | MemoryFlag::X)) return ".text";
		if (flags == (MemoryFlag::R | MemoryFlag::W)) return ".data";

		// weird sections
		if (flags == MemoryFlag::W) return ".w";
		if (flags == MemoryFlag::X) return ".x";
		if (flags == (MemoryFlag::W | MemoryFlag::X)) return ".wx";
		if (flags == (MemoryFlag::R | MemoryFlag::W | MemoryFlag::X)) return ".rwx";

		// flags == 0
		return ".nil";
	}

	/*
	 * class SegmentedBuffer
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

		throw std::runtime_error {"Undefined label '" + label.string() + "' used"};
	}

	void SegmentedBuffer::add_label(const Label& label) {
		auto it = labels.find(label);

		if (it == labels.end()) {
			labels[label] = sections[selected].current();
			return;
		}

		throw std::runtime_error {"Can't redefine label '" + label.string() + "', in section #" + std::to_string(selected)};
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

	void SegmentedBuffer::use_section(MemoryFlags flags, const std::string& hint) {
		int index = -1;
		const int count = static_cast<int>(sections.size());
		const std::string name = hint.empty() ? BufferSegment::default_name(flags) : hint;

		for (int i = 0; i < count; i ++) {
			const BufferSegment& segment = sections[i];

			// unless we specify a custom name matching by flags will be sufficient
			if (segment.flags == flags && segment.name == name) {
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
		sections.emplace_back(count, flags, name);
	}

	size_t SegmentedBuffer::count() const {
		return sections.size();
	}

	size_t SegmentedBuffer::total() const {
		auto last = sections.back();
		return last.start + last.buffer.size() + last.tail;
	}

	void SegmentedBuffer::dump() const {
		std::cout << "./unasm.sh " << base_address << " \"";

		for (const BufferSegment& segment : sections) {
			std::cout << "SECTION " << (segment.flags.x ? ".text" : ".data") << " \\ndb ";
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

	const std::vector<ExportSymbol>& SegmentedBuffer::exports() const {
		return exported_symbols;
	}

	void SegmentedBuffer::add_export(const Label& label, ExportSymbol::Type type, size_t size)  {
		exported_symbols.emplace_back(label, size, type);
	}

	void SegmentedBuffer::add_location(const std::string& path, uint32_t line, uint32_t column) {
		if (column > 0xFFFF) {
			column = 0xFFFF;
		}

		source_locations.emplace_back(current(), line, column, source_files.put(path));
	}

	const std::vector<SourceLocation>& SegmentedBuffer::locations() const {
		return source_locations;
	}

	const std::vector<std::string>& SegmentedBuffer::files() const {
		return source_files.items();
	}

}
