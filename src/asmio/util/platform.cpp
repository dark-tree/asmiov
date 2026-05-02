#include "platform.hpp"
#include <asmio/elf/object.hpp>

#include "tmp.hpp"

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
#include <windows.h>

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

		// TODO implement, an actual, in-memory CreateProcess()
		// this is done in a non-perfect way, by first copying the image to a file
		// there is a way to do this correctly, but it is convoluted
		// https://groups.google.com/g/comp.os.ms-windows.programmer.win32/c/Md3GKPc279A/m/Ax3bYgXhpD8J
		// https://web.archive.org/web/20131115160730/http://www.security.org.sg/code/loadexe.html

		// TODO don't ignore environ
		(void) envp;

		util::TempFile temp;
		temp.write((uint8_t*) image, bytes);

		PROCESS_INFORMATION process_info;
		STARTUPINFO startup_info;

		ZeroMemory(&process_info, sizeof(PROCESS_INFORMATION));
		ZeroMemory(&startup_info, sizeof(STARTUPINFO));

		startup_info.cb = sizeof(STARTUPINFO);

		std::string command = temp.path() + " ";

		// start at 1 to ignore name
		for (int i = 1; true; i++) {
			const char* part = argv[i];

			if (part == nullptr) {
				break;
			}

			command += part;
		}

		if (!CreateProcess(
			nullptr,
			(TCHAR*) command.c_str(),
			nullptr,
			nullptr,
			true, // inherit handles
			0,
			nullptr, // use parent's environment
			nullptr, // use parent's current directory
			&startup_info,
			&process_info
		)) {
			return RunResult::error(1);
		}

		DWORD exit_code = 1;

		if (WaitForSingleObject(process_info.hProcess, 10000) == WAIT_TIMEOUT) {
			return RunResult::error(2);
		}

		GetExitCodeProcess(process_info.hProcess, &exit_code);

		CloseHandle(process_info.hProcess);
		CloseHandle(process_info.hThread);

		return RunResult::success(exit_code);

	}

	std::string call_shell(std::string cmd, const std::string& input) {

		// https://learn.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output
		// https://www.mirabulus.com/it/blog/2021/05/16/hidden-issues-with-inheritable-handles-in-windows

		SECURITY_ATTRIBUTES security_attr;
		security_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
		security_attr.bInheritHandle = TRUE;
		security_attr.lpSecurityDescriptor = nullptr;

		HANDLE child_stdin_read = nullptr;
		HANDLE child_stdin_write = nullptr;
		HANDLE child_stdout_read = nullptr;
		HANDLE child_stdout_write = nullptr;

		CreatePipe(&child_stdout_read, &child_stdout_write, &security_attr, 0);
		SetHandleInformation(child_stdout_read, HANDLE_FLAG_INHERIT, 0);

		CreatePipe(&child_stdin_read, &child_stdin_write, &security_attr, 0);
		SetHandleInformation(child_stdin_write, HANDLE_FLAG_INHERIT, 0);

		PROCESS_INFORMATION process_info;
		STARTUPINFO startup_info;

		ZeroMemory(&process_info, sizeof(PROCESS_INFORMATION));
		ZeroMemory(&startup_info, sizeof(STARTUPINFO));

		startup_info.cb = sizeof(STARTUPINFO);
		startup_info.hStdError = child_stdout_write;
		startup_info.hStdOutput = child_stdout_write;
		startup_info.hStdInput = child_stdin_read;
		startup_info.dwFlags |= STARTF_USESTDHANDLES;

		std::string command = "cmd.exe /C " + cmd;

		if (!CreateProcess(
			nullptr,
			(TCHAR*) command.c_str(),
			nullptr,
			nullptr,
			true, // inherit handles
			CREATE_SUSPENDED,
			nullptr, // use parent's environment
			nullptr, // use parent's current directory
			&startup_info,
			&process_info
		)) {
			throw std::runtime_error {"Failed to create process '" + cmd + "'!"};
		}

		SetHandleInformation(child_stdout_write, HANDLE_FLAG_INHERIT, 0);
		SetHandleInformation(child_stdin_read, HANDLE_FLAG_INHERIT, 0);

		// close handles used by the child process
		CloseHandle(child_stdout_write);
		CloseHandle(child_stdin_read);

		ResumeThread(process_info.hThread);

		const char* data = input.data();
		DWORD bytes = input.size();
		DWORD written = 0;

		while (bytes > 0) {
			WriteFile(child_stdin_write, data, bytes, &written, nullptr);
			bytes -= written;
		}

		CloseHandle(child_stdin_write);

		DWORD exit_code = 1;
		std::string output;

		WaitForSingleObject(process_info.hProcess, INFINITE);
		GetExitCodeProcess(process_info.hProcess, &exit_code);

		CloseHandle(process_info.hProcess);
		CloseHandle(process_info.hThread);

		constexpr DWORD size = 256;
		char buffer[size];
		DWORD read = 0;

		while (true) {
			bool success = ReadFile(child_stdout_read, buffer, size, &read, nullptr);

			if (!success || (read == 0)) {
				break;
			}

			output.append(buffer, buffer + read);
		}

		CloseHandle(child_stdout_read);

		std::string normalized;
		normalized.reserve(output.size());

		for (char c : output) {
			if (c != '\r') {
				normalized.push_back(c);
			}
		}

		return normalized;
	}

}

#endif