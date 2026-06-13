
#include "object.hpp"

#include <filesystem>
#include <unistd.h>
#include <asmio/util/platform.hpp>

namespace asmio {

	std::ostream& operator<<(std::ostream& os, RunStatus c) {
		switch (c) {
			case RunStatus::SUCCESS: return os << "SUCCESS";
			case RunStatus::ERROR: return os << "ERROR";
			default: return os << "INVALID";
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
			std::fstream output {path, std::ios::out | std::ios::trunc | std::ios::binary};

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
		return run_file_image(image.data(), image.size(), argv, envp);
	}

}
