#pragma once

#include "external.hpp"
#include "util.hpp"

using namespace asmio;

class Args {

	private:

		// defined
		std::unordered_map<std::string, int> argcs;
		std::vector<std::string> defined;

		// loaded
		std::unordered_map<std::string, std::vector<std::string>> values;
		std::vector<std::string> options;
		std::vector<std::string> trailing;

		int store(const std::string& value) {
			if (!contains(defined, value)) {
				printf("Unknown option '%s'!\n", value.c_str());
				printf("Try '--help' for more info!\n");
				exit(1);
			}

			options.push_back(value);
			return options.size() - 1;
		}

	public:

		Args& define(const char* name, int argc = 0) {
			defined.emplace_back(name);
			argcs[name] = argc;
			return *this;
		}
	
		void load(int argc, char** argv) {
			std::list<std::string> parts;
			std::vector<std::string> args;
			args.reserve(argc);

			int last = -1;

			static auto link = [&] (bool finalize) {

				if (last != -1) {
					auto& key = options[last];
					int argc = argcs[key];
					auto& vals = values[key];

					if (!vals.empty()) {
						printf("Invalid syntax, '%s' was already used!\n", key.c_str());
						printf("Try '--help' for more info!\n");
						exit(1);
					}

					if (parts.size() < argc) {
						printf("Invalid syntax, too few arguments given to '%s'!\n", key.c_str());
						printf("Try '--help' for more info!\n");
						exit(1);
					}

					if (!finalize && parts.size() > argc) {
						printf("Invalid syntax, too many arguments given to '%s'!\n", key.c_str());
						printf("Try '--help' for more info!\n");
						exit(1);
					}

					while (argc --> 0) {
						vals.push_back(parts.front());
						parts.pop_front();
					}
				}

				if (finalize) {
					while (!parts.empty()) {
						trailing.push_back(parts.front());
						parts.pop_front();
					}

					return;
				}

				if (last == -1 && !parts.empty() /* && !finalize */) {
					printf("Invalid syntax, expected an option or flag!\n");
					printf("Try '--help' for more info!\n");
					exit(1);
				}


			};

			for (int i = 1; i < argc; i ++) {
				std::string part {argv[i]};
				
				if (part.empty()) {
					continue;
				}

				args.push_back(part);
				
				// long option
				if (part.starts_with("--")) {
					link(false);
					last = store(part);
					continue;
				}

				// joined flags
				if (part.starts_with('-') && part.length() > 2) {
					std::string tmp {"-0"};
					link(false);

					for (int j = 1; j < part.size(); j ++) {
						tmp[1] = part[j];
						last = store(tmp);
					}
					continue;
				}

				// separate flag
				if (part.starts_with('-') && part.length() == 2) {
					link(false);
					last = store(part);
					continue;
				}

				parts.push_back(part);
			}

			link(true);
		}

		void undefine() {
			defined.clear();
			argcs.clear();
			defined.shrink_to_fit();
		}

		void clear() {
			undefine();

			options.clear();
			values.clear();
			trailing.clear();
		}

		bool has(const char* name) {
			return contains(options, name);
		}

		std::vector<std::string> get(const char* name) {
			try { return values.at(name); } catch (...) { return {}; }
		}

		std::vector<std::string>& tail(int length) {
			if (trailing.size() != length && length != -1) {
				printf("Invalid syntax, expected %d %s!\n", length, (length == 1) ? "file" : "files");
				printf("Try '--help' for more info!\n");
				exit(1);
			}

			return trailing;
		}
	
};
