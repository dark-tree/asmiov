#pragma once

#include <out/chunk/buffer.hpp>
#include "abbrev.hpp"

namespace asmio {

	struct DwarfEntry {
		ChunkBuffer* buffer;
		size_t offset;

		ChunkBuffer* operator->() const {
			return buffer;
		}
	};

	class DwarfInformation {

		private:

			ChunkBuffer* m_root;
			ChunkBuffer* m_buffer;
			std::shared_ptr<DwarfAbbreviations> m_abbrev;

		public:

			DwarfInformation(ChunkBuffer::Ptr& buffer, const std::shared_ptr<DwarfAbbreviations>& abbreviation) noexcept;

			DwarfEntry compile_unit(const std::string& name, const std::string& producer = "asmiov");
			DwarfEntry submit(DwarfObjectBuilder& builder, DwarfEntry& entry);

	};

}