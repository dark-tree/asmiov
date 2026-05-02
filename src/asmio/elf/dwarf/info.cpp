#include "info.hpp"

#include "lang.hpp"
#include "unit.hpp"
#include "version.hpp"

namespace asmio {

	/*
	 * class DwarfInformation
	 */

	DwarfInformation::DwarfInformation(ChunkBuffer::Ptr& buffer, const std::shared_ptr<DwarfAbbreviations>& abbreviation) noexcept {

		auto head = buffer->chunk();

		// See DWARF specification v5 section 7.5.1.1, page 200
		// Compilation unit header
		head->link<uint32_t>([=] { return buffer->size() - 4; });
		head->put<uint16_t>(DWARF_VERSION);
		head->put<uint8_t>(DwarfUnit::compile);
		head->put<uint8_t>(8); // address width
		head->put<uint32_t>(0); // abbrev offset

		m_root = buffer.get();
		m_buffer = buffer->chunk().get();
		m_abbrev = abbreviation;
		buffer->put<uint8_t>(0);
	}

	DwarfEntry DwarfInformation::compile_unit(const std::string& name, const std::string& producer) {
		auto unit = DwarfObjectBuilder::of(DwarfTag::compile_unit)
			.add(DwarfAttr::producer, DwarfForm::string)
			.add(DwarfAttr::name, DwarfForm::string)
			.add(DwarfAttr::stmt_list, DwarfForm::data4)
			.add(DwarfAttr::language, DwarfForm::data2)
			.children();

		DwarfEntry root {m_buffer, m_buffer->size() - m_root->offset()};

		auto entry = submit(unit, root);
		entry->write(producer);
		entry->write(name);
		entry->put<uint32_t>(0);
		entry->put<uint16_t>(DwarfLang::C99);

		return entry;
	}

	DwarfEntry DwarfInformation::submit(DwarfObjectBuilder& builder, DwarfEntry& entry) {
		const int id = m_abbrev->submit(builder);
		ChunkBuffer::Ptr chunk = entry->chunk();

		if (builder.has_children()) {
			m_buffer->put<uint8_t>(0);
		}

		chunk->put<UnsignedLeb128>(id);
		return {chunk.get(), chunk->offset() - m_root->offset()};
	}

}
