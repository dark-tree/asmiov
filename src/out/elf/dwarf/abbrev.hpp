#pragma once
#include <cstdint>
#include <unordered_set>
#include <util.hpp>
#include <vector>
#include <out/chunk/buffer.hpp>
#include <out/chunk/codecs.hpp>

#include "attribute.hpp"
#include "form.hpp"
#include "tag.hpp"

namespace asmio {

	struct PACKED DwarfFormatedAttributeEntry {
		DwarfAttr attribute;
		DwarfForm format;

		constexpr bool operator==(const DwarfFormatedAttributeEntry& other) const {
			return attribute == other.attribute && format == other.format;
		}
	};

	struct DwarfAbbrevHeader {
		uint32_t hash = 0;
		uint32_t size = 0;
		DwarfTag tag = static_cast<DwarfTag>(0);
		bool children = false;

		DwarfAbbrevHeader() = default;
		DwarfAbbrevHeader(DwarfTag tag) : tag(tag) {}

		bool operator==(const DwarfAbbrevHeader& other) const;

		void rehash(const DwarfFormatedAttributeEntry* attribute_array);
	};

	struct DwarfAbbreviation {
		DwarfAbbrevHeader head;
		const DwarfFormatedAttributeEntry* attributes = nullptr;

		bool internal = false;
		int64_t id = -1;

		DwarfAbbreviation() = default;
		DwarfAbbreviation(const DwarfAbbreviation& other) = delete;

		DwarfAbbreviation(const DwarfAbbrevHeader& head, const DwarfFormatedAttributeEntry* attributes)
			: head(head), attributes(attributes) {
		}

		DwarfAbbreviation(DwarfAbbreviation&& other) noexcept
			: head(other.head), attributes(other.attributes), internal(other.internal), id(other.id) {
			other.internal = false;
			other.attributes = nullptr;
		}

		~DwarfAbbreviation() {
			if (internal) delete[] attributes;
		}

		void internalize(int id, ChunkBuffer::Ptr& writer);

		bool operator==(const DwarfAbbreviation& other) const;

		struct Hash {
			size_t operator() (const DwarfAbbreviation& abbreviation) const noexcept { return abbreviation.head.hash; }
		};
	};

	class DwarfObjectBuilder {

		private:

			DwarfAbbrevHeader head;
			DwarfFormatedAttributeEntry attributes[util::enum_length<DwarfAttr>] {};

			DwarfObjectBuilder(DwarfTag tag)
				: head(tag) {
			}

		public:

			static DwarfObjectBuilder of(DwarfTag tag);

			DwarfObjectBuilder& children();

			DwarfObjectBuilder& add(DwarfAttr attribute, DwarfForm format);

			DwarfAbbreviation delegate();

			bool has_children();

	};

	class DwarfAbbreviations {

		private:

			int id = 1;
			ChunkBuffer::Ptr m_buffer;
			std::unordered_set<DwarfAbbreviation, DwarfAbbreviation::Hash> m_abbreviations;

		public:

			DwarfAbbreviations(ChunkBuffer::Ptr& buffer) noexcept;

			int submit(DwarfObjectBuilder& builder);

	};

}
