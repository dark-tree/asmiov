#pragma once

namespace test {

	inline std::string call_shell(std::string cmd) {
		std::string out;
		char buf[256];

		cmd += " 2>&1";
		FILE* pipe = popen(cmd.c_str(), "r");
		if (!pipe) return out;

		while (fgets(buf, sizeof(buf), pipe)) {
			out += buf;
		}

		pclose(pipe);
		return out;
	}

}