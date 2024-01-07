#include "external.hpp"
#include "util.hpp"
#include "args.hpp"
#include "stream.hpp"
#include "tokenizer.hpp"
#include "error.hpp"
#include <asm/x86/writer.hpp>
#include <asm/x86/emitter.hpp>

// private libs
#include <iostream>
#include <fstream>

using namespace asmio;

int main(int argc, char** argv) {

	Args args;

	args.define("-i").define("--stdin");
	args.define("-o", 1).define("--output", 1);
	args.define("-?").define("-h").define("--help");
	args.define("--version");

	args.load(argc, argv);
	args.undefine();

	if (args.has("--help") || args.has("-h") || args.has("-?")) {
		printf("Usage: tasml [options...] [file]\n");
		printf("Assemble given file into executable ELF\n\n");

		printf("  -i, --stdin    Read input from stdin, not file\n");
		printf("  -o, --output   Place the output into <file>\n");
		printf("  -h, --help     Display this help page and exit\n");
		printf("      --version  Display version information and exit\n");

		return 0;
	}

	if (args.has("--version")) {
		printf("Tool-Assisted Machine Language - TASML\n");
		printf("Version: " ASMIOV_VERSION "\n");
		printf("Source: " ASMIOV_SOURCE "\n");

		return 0;
	}

	std::string assembly;
	std::string input = "<stdin>"; // path
	std::string output = "a.out"; // path

	if (args.has("-o")) {
		output = args.get("-o").at(0);
	}

	if (args.has("--output")) {
		output = args.get("--output").at(0);
	}

	if (args.has("-o") && args.has("--output")) {
		printf("Invalid syntax, output redefined!");
		printf("Try '--help' for more info!\n");
		exit(1);
	}

	if (args.has("-i") || args.has("--stdin")) {
		char chr;

		// assert no tail
		args.tail(0);

		// load until end of input is reached
		while (std::cin >> chr){
			assembly.push_back(chr);
		}
	} else {
		input = args.tail(1).at(0);
		std::ifstream file {input};

		if (!file.is_open()) {
			printf("Failed to read input!\n");
			return 1;
		}

		// load file contents into string
		file.seekg(0, std::ios::end);
		assembly.reserve(file.tellg());
		file.seekg(0, std::ios::beg);
		assembly.assign(std::istreambuf_iterator<char> {file}, std::istreambuf_iterator<char> {});
	}

	ErrorHandler handler {input, true};

	try {

		// tokenize input
		std::vector<Token> tokens = tokenize(handler, assembly);
		TokenStream stream {tokens};
		assembly.clear();

		// check tokenization error
		if (!handler.ok()) {
			handler.dump();
			return 1;
		}

		// parse and assemble
		x86::BufferWriter writer;
		x86::parseBlock(handler, writer, stream);

		// check parsing error
		if (!handler.ok()) {
			handler.dump();
			return 1;
		}

		// write to output file
		if (!writer.bake_elf().save(output.c_str())) {
			printf("Failed to save output!\n");
			return 1;
		}

	} catch (std::runtime_error& error) {
		printf("%s\n", error.what());
		return 1;
	}

	return 0;
}