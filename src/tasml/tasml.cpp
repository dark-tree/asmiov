#include "external.hpp"
#include "util.hpp"
#include "args.hpp"
#include "stream.hpp"
#include "tokenizer.hpp"
#include "error.hpp"
#include <asm/x86/writer.hpp>
#include <asm/x86/emitter.hpp>
#include <out/elf/buffer.hpp>

// private libs
#include <iostream>
#include <fstream>

#define EXIT_TOKEN_ERROR 2
#define EXIT_PARSE_ERROR 3
#define EXIT_LINKE_ERROR 4

int main(int argc, char** argv) {

	tasml::Args args;

	args.define("-i").define("--stdin");
	args.define("-o", 1).define("--output", 1);
	args.define("--xansi");
	args.define("-?").define("-h").define("--help");
	args.define("--version");

	args.load(argc, argv);
	args.undefine();

	if (args.has("--help") || args.has("-h") || args.has("-?")) {
		printf("Usage: tasml [options...] [file]\n");
		printf("Assemble given file into executable ELF\n\n");

		printf("  -i, --stdin    Read input from stdin, not file\n");
		printf("  -o, --output   Place the output into <file>\n");
		printf("      --xansi    Disables colored output\n");
		printf("  -h, --help     Display this help page and exit\n");
		printf("      --version  Display version information and exit\n");

		return EXIT_OK;
	}

	if (args.has("--version")) {
		printf("Tool-Assisted Machine Language - TASML\n");
		printf("Version: " ASMIOV_VERSION "\n");
		printf("Source: " ASMIOV_SOURCE "\n");

		return EXIT_OK;
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
		return EXIT_ERROR;
	}

	if (args.has("-i") || args.has("--stdin")) {
		char chr;

		// assert no tail
		args.tail(0);

		// load until end of input is reached
		while (std::cin.get(chr)){
			assembly.push_back(chr);
		}
	} else {
		input = args.tail(1).at(0);
		std::ifstream file {input};

		if (!file.is_open()) {
			printf("Failed to read input!\n");
			return EXIT_ERROR;
		}

		// load file contents into string
		file.seekg(0, std::ios::end);
		assembly.reserve(file.tellg());
		file.seekg(0, std::ios::beg);
		assembly.assign(std::istreambuf_iterator<char> {file}, std::istreambuf_iterator<char> {});
	}

	tasml::ErrorHandler handler {input, !args.has("--xansi")};

	try {

		// tokenize input
		std::vector<tasml::Token> tokens = tokenize(handler, assembly);
		tasml::TokenStream stream {tokens}; assembly.clear();
		handler.assert(EXIT_TOKEN_ERROR);

		// parse and assemble
		asmio::SegmentedBuffer buffer;
		asmio::x86::BufferWriter writer {buffer};
		asmio::x86::parseBlock(handler, writer, stream);
		handler.assert(EXIT_PARSE_ERROR);

		// assemble buffer and create ELF file
		asmio::elf::ElfBuffer elf = asmio::elf::to_elf(buffer, "_start", DEFAULT_ELF_MOUNT, [&] (const auto& link, const char* what) {
			handler.link(link.target, what);
		});
		handler.assert(EXIT_LINKE_ERROR);

		// write to output file
		if (!elf.save(output.c_str())) {
			printf("Failed to save output!\n");
			return EXIT_ERROR;
		}

	} catch (std::runtime_error& error) {
		printf("Unhandled Error: %s\n", error.what());
		return EXIT_ERROR;
	}

	return EXIT_OK;
}