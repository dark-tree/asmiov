#pragma once

#include "external.hpp"
#include "label.hpp"

namespace asmio {

	enum LinkType {
		RELATIVE,
		ABSOLUTE,
	};

	template<typename T>
	using LabelLookup = std::unordered_map<Label, T, Label::HashFunction>;


	struct LabelMarker {
		uint32_t section;
		uint32_t offset;
	};

	struct BufferSection {

		constexpr static uint8_t R = 0b001;
		constexpr static uint8_t W = 0b010;
		constexpr static uint8_t X = 0b100;

		// by default create a mixed-use section
		constexpr static uint8_t DEFAULT = R | W | X;

		uint32_t index = 0;
		uint8_t flags = 0;
		uint8_t padder = 0; // byte used to pad the buffer tail
		std::vector<uint8_t> buffer;

		// set only once aligned, no data must be written after that point
		int64_t start = 0;
		int64_t tail = 0;

		BufferSection(uint32_t index, uint8_t flags)
			: index(index), flags(flags) {
		}

		size_t size() const {
			return buffer.size() + tail;
		}

		LabelMarker current() const {
			return {index, static_cast<uint32_t>(buffer.size())};
		}

		size_t align(size_t start, size_t page) {
			this->start = start;
			const int64_t bytes = buffer.size();
			const int64_t aligned = ALIGN_UP(bytes, page);

			// extra bytes to pad to page boundary
			this->tail = aligned - bytes;
			return aligned;
		}

		/// Convert internal flags to the mprotect flags
		/// FIXME maybe let's not do this
		int get_mprot_flags() const {
			int protect = 0;
			if (flags & R) protect |= PROT_READ;
			if (flags & W) protect |= PROT_WRITE;
			if (flags & X) protect |= PROT_EXEC;
			return protect;
		}

	};

	struct Linkage {

		enum LinkType {
			RELATIVE,
			ABSOLUTE,
		};

		using Linker = std::function<void(class SegmentedBuffer* buffer, const Linkage& link, size_t mount)>;

		Label label;
		LabelMarker target;
		Linker linker;

	};

	class SegmentedBuffer {

		private:

			int selected = 0;
			std::vector<BufferSection> sections;
			LabelLookup<LabelMarker> labels;
			std::vector<Linkage> linkages;

		public:

			int64_t get_offset(LabelMarker marker) const {
				return sections.at(marker.section).start + marker.offset;
			}

			uint8_t* get_pointer(LabelMarker marker) {
				return sections.at(marker.section).buffer.data() + marker.offset;
			}

			void align(size_t page) {
				size_t offset = 0;

				// align sections to page boundaries
				for (BufferSection& section : sections) {
					offset = section.align(offset, page);
				}
			}

			/// Execute all linkages
			void link(size_t base) {
				for (const Linkage& linkage : linkages) {
					// try {
						linkage.linker(this, linkage, base);
					// } catch (std::runtime_error& error) {
					// 	if (reporter != nullptr) reporter->link(command.offset, error.what()); else throw;
					// }
				}
			}

			/// Insert linker command
			void add_linkage(const Label& label, int shift, const Linkage::Linker& linker) {
				uint32_t offset = sections[selected].buffer.size();
				linkages.emplace_back(label, LabelMarker {(uint32_t) selected, offset + shift}, linker);
			}

			/// Get the label value
			LabelMarker get_label(const Label& label) {
				auto it = labels.find(label);

				if (it != labels.end()) {
					return it->second;
				}

				throw std::runtime_error {"Undefined label '" + std::string(label.c_str()) + "' used"};
			}

			/// Add the given label into the buffer
			void add_label(const Label& label) {
				auto it = labels.find(label);

				if (it == labels.end()) {
					labels[label] = sections[selected].current();
					return;
				}

				throw std::runtime_error {"Can't redefine label '" + std::string(label.c_str()) + "', in section #" + std::to_string(selected)};
			}

			/// Check if given labels have already been defined
			bool has_label(const Label& label) {
				return labels.contains(label);
			}

			/// Append a single byte to the current section
			void push(uint8_t byte) {
				auto& buffer = sections[selected].buffer;
				buffer.push_back(byte);
			}

			/// Append N bytes of a uniform value to the current section
			void fill(int64_t bytes, uint8_t value) {
				auto& buffer = sections[selected].buffer;
				buffer.resize(buffer.size() + bytes, value);
			}

			/// Append arbitrary data into the current section
			void insert(uint8_t* data, size_t bytes) {
				auto& buffer = sections[selected].buffer;
				buffer.insert(buffer.end(), data, data + bytes);
			}

			/// Select the section to use
			void use_section(uint8_t flags) {
				int index = -1;
				const uint32_t count = sections.size();

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

			/// Get section count
			size_t count() {
				return sections.size();
			}

			/// Get the total size in bytes of the whole segmented buffer, can be used only after linking
			size_t total() {
				auto last = sections.back();
				return last.start + last.buffer.size() + last.tail;
			}

			const std::vector<BufferSection>& segments() {
				return sections;
			}

			LabelLookup<size_t> resolved_labels() const {
				LabelLookup<size_t> result;

				for (auto& [label, marker] : labels) {
					result[label] = get_offset(marker);
				}

				return result;
			}

			SegmentedBuffer() {
				use_section(BufferSection::DEFAULT);
			}

	};

}
