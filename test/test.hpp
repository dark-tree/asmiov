#pragma once

#include <string>
#include <unistd.h>
#include <wait.h>

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

	inline void dump(asmio::SegmentedBuffer& buffer) {

		if (buffer.elf_machine == asmio::ElfMachine::NONE) {
			buffer.elf_machine = asmio::ElfMachine::NATIVE;
		}

		asmio::ObjectFile baked = asmio::to_elf(buffer, asmio::Label::UNSET).bake();
		asmio::util::TempFile temp {baked};

		std::string out = call_shell("objdump --visualize-jumps -wxd -Mintel " + temp.path());
		printf("%s\n", out.c_str());

		printf("\nAuto-generated assertions:\n\n");
		printf("\tbuffer.link(0);\n");

		int i = 0;

		for (auto& segment : buffer.segments()) {
			int count = segment.buffer.size();

			if (count == 0) {
				i ++;
				continue;
			}

			count --;

			printf("\tstd::vector<uint8_t> s%d = {", i);

			for (int j = 0; j <= count; j ++) {
				printf("0x%02x", segment.buffer[j]);

				if (count != j) {
					printf(", ");
				}
			}

			printf("};\n");
			printf("\tCHECK(buffer.segments()[%d].buffer, s%d); // %s\n\n", i, i, segment.name.c_str());
			i ++;
		}

	}

}