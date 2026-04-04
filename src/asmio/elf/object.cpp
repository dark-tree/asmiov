
#include "object.hpp"

#include <filesystem>

namespace asmio {

	std::ostream& operator<<(std::ostream& os, RunStatus c) {
		switch (c) {
			case RunStatus::SUCCESS: return os << "SUCCESS";
			case RunStatus::ARGS_ERROR: return os << "ARGS_ERROR";
			case RunStatus::MEMFD_ERROR: return os << "MEMFD_ERROR";
			case RunStatus::SEAL_ERROR: return os << "SEAL_ERROR";
			case RunStatus::FORK_ERROR: return os << "FORK_ERROR";
			case RunStatus::EXEC_ERROR: return os << "EXEC_ERROR";
			case RunStatus::WAIT_ERROR: return os << "WAIT_ERROR";
			default: return os << "UNKNOWN";
		}
	}

	std::ostream& operator<<(std::ostream& os, const RunResult& result) {
		return os << "RunResult{status=" << result.type << ", return=" << result.status << "}";
	}

	/*
	 * class ObjectFile
	 */

	ObjectFile::ObjectFile(std::vector<uint8_t>&& image)
		: image(std::move(image)) {
	}

	ObjectFile::ObjectFile(const std::vector<uint8_t>& image)
		: image(image) {
	}

	bool ObjectFile::save(const std::string& path) const {
		using std::filesystem::perms;

		// if file creation fails return false
		try {
			std::ofstream output {path};

			if (output.bad()) {
				return false;
			}

			output.write(reinterpret_cast<const char*>(image.data()), image.size());
			output.close();
		} catch (const std::exception&) {
			return false;
		}

		// this part is best-effort only
		try {
			const perms flags = perms::owner_exec | perms::group_exec | perms::others_exec;
			std::filesystem::permissions(path, flags, std::filesystem::perm_options::add);
		} catch (const std::exception&) {}

		// file was created
		return true;
	}

	const std::vector<uint8_t>& ObjectFile::bytes() const {
		return image;
	}

	RunResult ObjectFile::execute(const char* name) const {
		const char* argv[] = {name, nullptr};
		return execute(argv, const_cast<const char**>(environ));
	}

	RunResult ObjectFile::execute(const char** argv, const char** envp) const {
		// verify arguments, status can be a nullptr
		if (argv == nullptr || envp == nullptr) {
			return RunStatus::ARGS_ERROR;
		}

		// create in-memory file descriptor
		const int memfd = memfd_create("buffer", MFD_ALLOW_SEALING | MFD_CLOEXEC);
		if (memfd == -1) {
			return RunStatus::MEMFD_ERROR;
		}

		int* flag = (int*) mmap(nullptr, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if (flag == nullptr) {
			return RunStatus::MMAP_ERROR;
		}

		// copy buffer into memfd
		write(memfd, image.data(), image.size());

		// we use this to check if the child really run or did execve just fail
		*flag = 0;

		// add seals to memfd
		if (fcntl(memfd, F_ADD_SEALS, F_SEAL_WRITE | F_SEAL_GROW | F_SEAL_SHRINK | F_SEAL_SEAL) != 0) {
			return RunStatus::SEAL_ERROR;
		}

		const pid_t pid = fork();
		if (pid == -1) {
			return RunStatus::FORK_ERROR;
		}

		// replace child with memfd elf file
		if (pid == 0) {
			fexecve(memfd, const_cast<char* const*>(argv), const_cast<char* const*>(envp));

			// if fexecve fails we need to kill ourselves
			*flag = 1;
			exit(1);
		}

		int status = 0;

		// wait for child and get status code
		if (waitpid(pid, &status, 0) == -1) {
			return RunStatus::WAIT_ERROR;
		}

		if (*flag) {
			return RunStatus::EXEC_ERROR;
		}

		munmap(flag, sizeof(int));

		// obtain return code from child status
		return WEXITSTATUS(status);
	}

}
