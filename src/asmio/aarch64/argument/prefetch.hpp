#pragma once
#include <stdexcept>
#include <string_view>

namespace asmio::arm {

	// The Prefetch enum specifies the prefetch hint as follows:
	//
	// Access type:
	// - PLD for prefetch for load data.
	// - PLI for prefetch for load instruction.
	// - PST for prefetch for store.
	//
	// Target cache level:
	// - L1 for Level 1 cache.
	// - L2 for Level 2 cache.
	// - L3 for Level 3 cache.
	// - SLC for system level cache (requires FEAT_PRFMSLC support).
	//
	// Policy:
	// - KEEP for retained or temporal prefetch, allocated in the cache normally.
	// - STRM for streaming or non-temporal prefetch, for data that is used only once.
	enum struct Prefetch {

		PLD_L1_KEEP  = 0b00000,
		PLD_L1_STRM  = 0b00001,
		PLD_L2_KEEP  = 0b00010,
		PLD_L2_STRM  = 0b00011,
		PLD_L3_KEEP  = 0b00100,
		PLD_L3_STRM  = 0b00101,
		PLD_SLC_KEEP = 0b00110,
		PLD_SLC_STRM = 0b00111,
		PLI_L1_KEEP  = 0b01000,
		PLI_L1_STRM  = 0b01001,
		PLI_L2_KEEP  = 0b01010,
		PLI_L2_STRM  = 0b01011,
		PLI_L3_KEEP  = 0b01100,
		PLI_L3_STRM  = 0b01101,
		PLI_SLC_KEEP = 0b01110,
		PLI_SLC_STRM = 0b01111,
		PST_L1_KEEP  = 0b10000,
		PST_L1_STRM  = 0b10001,
		PST_L2_KEEP  = 0b10010,
		PST_L2_STRM  = 0b10011,
		PST_L3_KEEP  = 0b10100,
		PST_L3_STRM  = 0b10101,
		PST_SLC_KEEP = 0b10110,
		PST_SLC_STRM = 0b10111,

	};

	inline Prefetch parse_prefetch_enum(const std::string_view& view) {
		if (view == "PLD_L1_KEEP") return Prefetch::PLD_L1_KEEP;
		if (view == "PLD_L1_STRM") return Prefetch::PLD_L1_STRM;
		if (view == "PLD_L2_KEEP") return Prefetch::PLD_L2_KEEP;
		if (view == "PLD_L2_STRM") return Prefetch::PLD_L2_STRM;
		if (view == "PLD_L3_KEEP") return Prefetch::PLD_L3_KEEP;
		if (view == "PLD_L3_STRM") return Prefetch::PLD_L3_STRM;
		if (view == "PLD_SLC_KEEP") return Prefetch::PLD_SLC_KEEP;
		if (view == "PLD_SLC_STRM") return Prefetch::PLD_SLC_STRM;
		if (view == "PLI_L1_KEEP") return Prefetch::PLI_L1_KEEP;
		if (view == "PLI_L1_STRM") return Prefetch::PLI_L1_STRM;
		if (view == "PLI_L2_KEEP") return Prefetch::PLI_L2_KEEP;
		if (view == "PLI_L2_STRM") return Prefetch::PLI_L2_STRM;
		if (view == "PLI_L3_KEEP") return Prefetch::PLI_L3_KEEP;
		if (view == "PLI_L3_STRM") return Prefetch::PLI_L3_STRM;
		if (view == "PLI_SLC_KEEP") return Prefetch::PLI_SLC_KEEP;
		if (view == "PLI_SLC_STRM") return Prefetch::PLI_SLC_STRM;
		if (view == "PST_L1_KEEP") return Prefetch::PST_L1_KEEP;
		if (view == "PST_L1_STRM") return Prefetch::PST_L1_STRM;
		if (view == "PST_L2_KEEP") return Prefetch::PST_L2_KEEP;
		if (view == "PST_L2_STRM") return Prefetch::PST_L2_STRM;
		if (view == "PST_L3_KEEP") return Prefetch::PST_L3_KEEP;
		if (view == "PST_L3_STRM") return Prefetch::PST_L3_STRM;
		if (view == "PST_SLC_KEEP") return Prefetch::PST_SLC_KEEP;
		if (view == "PST_SLC_STRM") return Prefetch::PST_SLC_STRM;

		throw std::runtime_error {"Invalid prefetch enumeration"};
	}

}
