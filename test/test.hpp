#pragma once

#include <string>

namespace test {

	inline std::string call_shell(std::string cmd, const std::string& input = "") {

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