#pragma once

#include "external.hpp"
#include "label.hpp"

namespace asmio {

	/// Universal SegmentedBuffer data pointer
	struct BufferMarker {
		uint32_t section;
		uint32_t offset;
	};

	/// Single link job entry
	struct Linkage {

		using Linker = std::function<void(class SegmentedBuffer* buffer, const Linkage& link, size_t mount)>;

		Label label;
		BufferMarker target;
		Linker linker;

	};

	/// One track in the SegmentedBuffer
	struct BufferSegment {

		constexpr static uint8_t R = 0b001;
		constexpr static uint8_t W = 0b010;
		constexpr static uint8_t X = 0b100;

		// by default create a mixed-use section
		constexpr static uint8_t DEFAULT = R | W | X;

		uint16_t index = 0;
		uint8_t flags = 0;
		uint8_t padder = 0; // byte used to pad the buffer tail
		std::vector<uint8_t> buffer;

		// set only once aligned, no data must be written after that point
		int64_t start = 0;
		int64_t tail = 0;

		BufferSegment(uint32_t index, uint8_t flags);

		/// Get size of this buffer, including padding
		size_t size() const;

		/// Return a LabelMarker that points to the current buffer position
		BufferMarker current() const;

		/// Update internal paddings to ensure page alignment
		size_t align(size_t start, size_t page);

		/// Convert internal flags to the mprotect() flags set
		int get_mprot_flags() const;

	};

	/// Multi-track buffer, section and segment are used quite interchangeably here
	class SegmentedBuffer {

		private:

			size_t base_address = 0; // this is set during linking and used ONLY for debugging
			int selected = 0;
			std::vector<BufferSegment> sections;
			LabelMap<BufferMarker> labels;
			std::vector<Linkage> linkages;

		public:

			SegmentedBuffer();

			/// Get marker of the current buffer segment position
			BufferMarker current() const;

			/// offset in the final contiguous buffer of the given marker
			int64_t get_offset(BufferMarker marker) const;

			/// Get a pointer into one of the sections where the marker points
			uint8_t* get_pointer(BufferMarker marker);

			/// Needs to be called before linking, calculates sections start/end offsets
			void align(size_t page);

			/// Execute all linkages
			void link(size_t base);

			/// Insert linker command to be executed once link() is called
			void add_linkage(const Label& label, int shift, const Linkage::Linker& linker);

			/// Get the label value
			BufferMarker get_label(const Label& label);

			/// Add the given label into the buffer
			void add_label(const Label& label);

			/// Check if given labels have already been defined
			bool has_label(const Label& label);

			/// Append a single byte to the current section
			void push(uint8_t byte);

			/// Append N bytes of a uniform value to the current section
			void fill(int64_t bytes, uint8_t value);

			/// Append arbitrary data into the current section
			void insert(uint8_t* data, size_t bytes);

			/// Select the section to use
			void use_section(uint8_t flags);

			/// Get section count
			size_t count();

			/// Get the total size in bytes of the whole segmented buffer, can be used only after linking
			size_t total();

			/// Print the contests of this buffer for debugging
			void dump() const;

			/// Get segment list
			const std::vector<BufferSegment>& segments() const;

			/// Get a copy of the label map
			LabelMap<size_t> resolved_labels() const;

	};

}
