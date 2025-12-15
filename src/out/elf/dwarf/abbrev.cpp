#include "abbrev.hpp"

namespace asmio {

	/*
	 * region DwarfAbbrevHeader
	 */

	bool DwarfAbbrevHeader::operator==(const DwarfAbbrevHeader& other) const {
		if (other.hash != hash) return false;
		if (other.size != size) return false;
		if (other.tag != tag) return false;
		if (other.children != children) return false;

		return true;
	}

	void DwarfAbbrevHeader::rehash(const DwarfFormatedAttributeEntry* attribute_array) {
		hash = util::hash_djb2(reinterpret_cast<const char*>(attribute_array), size);
		hash ^= (static_cast<int>(tag) * 31) ^ (children * 109);
	}

	/*
	 * region DwarfAbbreviation
	 */

	void DwarfAbbreviation::internalize(int id, ChunkBuffer::Ptr& writer) {
		auto* buffer = new DwarfFormatedAttributeEntry[head.size];
		memcpy(buffer, attributes, head.size * sizeof(DwarfFormatedAttributeEntry));
		this->attributes = buffer;
		this->internal = true;
		this->id = id;

		// See DWARF Specification v5 section 7.5.3, page 203
		writer->put<UnsignedLeb128>(id);
		writer->put<UnsignedLeb128>(head.tag);
		writer->put<uint8_t>(head.children);

		for (size_t i = 0; i < head.size; i++) {
			auto entry = attributes[i];

			writer->put<UnsignedLeb128>(entry.attribute);
			writer->put<UnsignedLeb128>(entry.format);
		}

		writer->put<uint8_t>(0);
		writer->put<uint8_t>(0);
	}

	bool DwarfAbbreviation::operator==(const DwarfAbbreviation& other) const {
		if (other.head != head) return false;

		for (size_t i = 0; i < head.size; i++) {
			if (attributes[i] != other.attributes[i]) return false;
		}

		return true;
	}

	/*
	 * region DwarfObjectBuilder
	 */

	DwarfObjectBuilder DwarfObjectBuilder::of(DwarfTag tag) {
		return {tag};
	}

	DwarfObjectBuilder& DwarfObjectBuilder::children() {
		head.children = true;
		return *this;
	}

	DwarfObjectBuilder& DwarfObjectBuilder::add(DwarfAttr attribute, DwarfForm format) {
		attributes[head.size ++] = {attribute, format};
		return *this;
	}

	DwarfAbbreviation DwarfObjectBuilder::delegate() {
		head.rehash(attributes);

		DwarfAbbreviation abbreviation;
		abbreviation.head = head;
		abbreviation.attributes = reinterpret_cast<const DwarfFormatedAttributeEntry*>(&attributes);
		return abbreviation;
	}

	/*
	 * region DwarfAbbreviations
	 */

	DwarfAbbreviations::DwarfAbbreviations(ChunkBuffer::Ptr& buffer) noexcept {
		m_buffer = buffer->chunk();
		buffer->put<uint8_t>(0);
	}

	int DwarfAbbreviations::submit(DwarfObjectBuilder& builder) {
		DwarfAbbreviation abbreviation = builder.delegate();

		auto it = m_abbreviations.find(abbreviation);
		if (it != m_abbreviations.end()) {
			return it->id;
		}

		abbreviation.internalize(id ++, m_buffer);
		return m_abbreviations.emplace(std::move(abbreviation)).first->id;
	}

}