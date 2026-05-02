#pragma once

#include <asmio/external.hpp>
#include "dwarf/abbrev.hpp"

#define DEFAULT_ELF_MOUNT 0x08048000

namespace asmio {

	enum struct RunStatus {
		SUCCESS,
		ERROR,
	};

	struct RunResult {
		constexpr RunResult(RunStatus type, int status)
			: type(type), status(status) {
		}

		/// Create result with a specific error code
		constexpr static RunResult error(int error_status) {
			return {RunStatus::ERROR, error_status};
		}

		/// Create a result with a specific return code
		constexpr static RunResult success(int return_status) {
			return {RunStatus::SUCCESS, return_status};
		}

		const RunStatus type;
		const int status;
	};

	std::ostream& operator<<(std::ostream& os, RunStatus c);
	std::ostream& operator<<(std::ostream& os, const RunResult& result);

	class ObjectFile {

		private:

			std::vector<uint8_t> image;

		public:

			ObjectFile() = default;
			ObjectFile(std::vector<uint8_t>&& image);
			ObjectFile(const std::vector<uint8_t>& image);

			/**
			 * Save the object to an executable file,
			 * if that is possible the file is given the execute permission.
			 */
			bool save(const std::string& path) const;

			/**
			 * Serialize the object file to a byte buffer,
			 * if you wish to save the file use save() instead.
			 */
			const std::vector<uint8_t>& bytes() const;

			/**
			 * Fork into the in-memory view of the file,
			 * environ is inherited from the calling process.
			 */
			RunResult execute(const char* name) const;

			/**
			 * Fork into the in-memory view of the file, with arguments,
			 * to inherit environ pass it as the second argument.
			 */
			RunResult execute(const char** argv, const char** envp) const;

	};

}
