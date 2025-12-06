#include "tmp.hpp"

#include <util.hpp>

namespace asmio::util {

	/*
	 * class TempFile
	 */

	TempFile::TempFile(const char* extension) {
		auto tmp = std::filesystem::temp_directory_path() / "asmiov";
		std::filesystem::create_directories(tmp);
		m_path = tmp / (random_string(10) + extension);
	}

	TempFile::~TempFile() {
		if (auto_delete) {
			std::filesystem::remove(m_path);
		}
	}

	void TempFile::retain() {
		auto_delete = false;
	}

	void TempFile::dump() const {
		printf("Using temporary file: \"%s\"\n", m_path.c_str());
	}

	std::string TempFile::path() const {
		return m_path;
	}

}
