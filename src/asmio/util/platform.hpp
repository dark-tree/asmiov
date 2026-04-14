#pragma once
#include <asmio/program/memory.hpp>

namespace asmio {

	struct RunResult;

	/// Get size of one page in bytes
	uint32_t page_size();

	/// Allocate memory in pages, in a way that it can be used by protect_pages()
	void* allocate_pages(uint64_t bytes, MemoryFlags initial);

	/// Change permissions assigned to memory pages
	void protect_pages(void* page, uint64_t bytes, MemoryFlags flags);

	/// Free memory allocated with allocate_pages()
	void free_pages(void* page, uint64_t bytes);

	/// Run executable file image
	RunResult run_file_image(const void* image, size_t bytes, const char** argv, const char** envp);

	/// Invoke the given command, run it with the given STDIN, and return STDOUT
	std::string call_shell(std::string cmd, const std::string& input = "");

}
