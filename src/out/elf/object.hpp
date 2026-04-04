#pragma once

#include "external.hpp"
#include "dwarf/abbrev.hpp"

#define DEFAULT_ELF_MOUNT 0x08048000

namespace asmio {

	template <auto V>
	constexpr static auto supply = [] noexcept { return V; };

	enum struct RunStatus {
		SUCCESS,     ///< elf file was executed
		ARGS_ERROR,  ///< the given arguments are invalid
		MEMFD_ERROR, ///< memfd failed
		MMAP_ERROR,  ///< mmap failed
		SEAL_ERROR,  ///< fcntl failed
		STAT_ERROR,  ///< fstat failed
		FORK_ERROR,  ///< fork failed
		EXEC_ERROR,  ///< file not executable
		WAIT_ERROR,  ///< waitpid failed
	};

	struct RunResult {
		RunResult(RunStatus type)
			: type(type), status(0) {
		}

		RunResult(int status)
			: type(RunStatus::SUCCESS), status(status) {
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
