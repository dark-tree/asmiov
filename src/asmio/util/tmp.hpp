#pragma once

#include <asmio/external.hpp>

namespace asmio::util {

	class TempFile {

		private:

			bool auto_delete = true;
			std::filesystem::path m_path;

		public:

			void retain();
			void dump() const;
			std::string path() const;

			template <typename T>
			TempFile(const T& savable, const char* extension = "") : TempFile(extension) {
				(void) savable.save(path());
			}

			void write(const std::string& content) {
				std::fstream out(path(), std::ios::out | std::ios::trunc | std::ios::binary);
				out << content;
				out.close();
			}

			TempFile(const char* extension = "");
			~TempFile();

	};

}
