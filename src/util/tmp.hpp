#pragma once
#include <string>
#include <filesystem>

namespace asmio::util {

	class TempFile {

		private:

			bool auto_delete = true;
			std::filesystem::path m_path;

		public:

			void retain();
			void dump() const;
			std::string path() const;

			TempFile();
			~TempFile();

	};

}
