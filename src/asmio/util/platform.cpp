#include "platform.hpp"
#include <asmio/elf/object.hpp>

#ifdef __linux__

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>

namespace asmio {

	uint32_t page_size() {
		return getpagesize();
	}

	void* allocate_pages(uint64_t bytes, MemoryFlags initial) {
		return mmap(nullptr, bytes, initial.to_mprotect(), MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	}

	void protect_pages(void* page, uint64_t bytes, MemoryFlags flags) {
		mprotect(page, bytes, flags.to_mprotect());
	}

	void free_pages(void* page, uint64_t bytes) {
		munmap(page, bytes);
	}

	RunResult run_file_image(const void* image, size_t bytes, const char** argv, const char** envp) {
		// verify arguments
		if (argv == nullptr || envp == nullptr) {
			return RunResult::error(1);
		}

		// create in-memory file descriptor
		const int memfd = memfd_create("buffer", MFD_ALLOW_SEALING | MFD_CLOEXEC);
		if (memfd == -1) {
			return RunResult::error(2);
		}

		int* flag = (int*) mmap(nullptr, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if (flag == nullptr) {
			return RunResult::error(3);
		}

		// copy buffer into memfd
		write(memfd, image, bytes);

		// we use this to check if the child really run or did execve just fail
		*flag = 0;

		// add seals to memfd
		if (fcntl(memfd, F_ADD_SEALS, F_SEAL_WRITE | F_SEAL_GROW | F_SEAL_SHRINK | F_SEAL_SEAL) != 0) {
			return RunResult::error(4);
		}

		const pid_t pid = fork();
		if (pid == -1) {
			return RunResult::error(5);
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
			return RunResult::error(6);
		}

		if (*flag) {
			return RunResult::error(7);
		}

		free_pages(flag, sizeof(int));

		// obtain return code from child status
		return RunResult::success(WEXITSTATUS(status));
	}

	std::string call_shell(std::string cmd, const std::string& input) {

		// refers to the stdin/stdout of the child process
		// [0] read, [1] write
		int pipe_stdin[2];
		int pipe_stdout[2];

		pipe2(pipe_stdin, 0);
		pipe2(pipe_stdout, 0);

		pid_t pid = fork();

		if (pid == 0) {
			dup2(pipe_stdin[0], STDIN_FILENO);
			dup2(pipe_stdout[1], STDOUT_FILENO);
			dup2(pipe_stdout[1], STDERR_FILENO);

			// close the unused end
			close(pipe_stdin[1]);
			close(pipe_stdout[0]);

			execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
			exit(1);
		}

		// close the unused end
		close(pipe_stdin[0]);
		close(pipe_stdout[1]);

		if (!input.empty()) {
			write(pipe_stdin[1], input.data(), input.size());
		}

		close(pipe_stdin[1]);

		std::string out;
		char buf[256];
		ssize_t n;

		while ((n = read(pipe_stdout[0], buf, sizeof(buf))) > 0) {
			out.append(buf, n);
		}

		close(pipe_stdout[0]);
		waitpid(pid, nullptr, 0);
		return out;
	}

}

#endif

#ifdef _WIN32
#include <sysinfoapi.h>
#include <memoryapi.h>

namespace asmio {

	uint32_t page_size() {
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		return info.dwPageSize;
	}

	void* allocate_pages(uint64_t bytes, MemoryFlags initial) {
		return VirtualAlloc(nullptr, bytes, MEM_RESERVE | MEM_COMMIT, initial.to_win32());
	}

	void protect_pages(void* page, uint64_t bytes, MemoryFlags flags) {
		DWORD unused;
		VirtualProtect(page, bytes, flags.to_win32(), &unused);
	}

	void free_pages(void* page, uint64_t bytes) {
		VirtualFree(page, 0, MEM_RELEASE);
	}

	RunResult run_file_image(const void* image, size_t bytes, const char** argv, const char** envp) {
		throw std::runtime_error {"Operation run_file_image() not supported on this platform!"};
	}

	std::string call_shell(std::string cmd, const std::string& input) {
		throw std::runtime_error {"Operation call_shell() not supported on this platform!"};
	}

}

#endif