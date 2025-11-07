#pragma once

#include "external.hpp"
#include "out/buffer/segmented.hpp"

namespace tasml {

	struct ErrorHandler {

		public:

			class Report {

				public:

					enum Type : uint32_t {
						WARNING,
						ERROR,
						FATAL,
						LINK
					};

					Report(Type type, uint32_t line, uint32_t column, const std::string& message, const std::string& unit)
					: line(line), column(column), type(type), message(message), unit(unit) {}

					const char* typestr(bool ansi) const {
						if (type == WARNING) return ansi ? "\033[33;1mWarning:\033[0m" : "Warning:";
						if (type == ERROR) return ansi ? "\033[31;1mError:\033[0m" : "Error:";
						if (type == FATAL) return ansi ? "\033[31;1mFatal Error:\033[0m" : "Fatal Error:";
						if (type == LINK) return ansi ? "\033[31;1mLink Error:\033[0m" : "Link Error:";

						return ansi ? "\033[31;1mInvalid:\033[0m" : "Invalid:";
					}

					void dump(bool ansi) const {
						if (type == LINK) {
							printf("%s at %d+0x%08x %s %s!\n", unit.c_str(), line, column, typestr(ansi), message.c_str());
							return;
						}

						printf("%s:%d %s %s!\n", unit.c_str(), line, typestr(ansi), message.c_str());
					}

				private:

					uint32_t line;
					uint32_t column;
					Type type;
					std::string message;
					std::string unit;

			};

			ErrorHandler(const std::string& unit, bool ansi)
			: unit(unit), ansi(ansi), errors(0), warnings(0) {}

			bool ok() const {
				return errors == 0;
			}

			void dump() {
				for (Report report : reports) {
					report.dump(ansi);
				}

				reports.clear();
				warnings = 0;
				errors = 0;
			}

			void assert(int code) {
				const bool failed = !ok();
				dump();

				if (failed) {
					exit(code);
				}
			}

		public:

			void warn(int line, int column, const std::string& message) {
				reports.emplace_back(Report::WARNING, line, column, message, unit);
				warnings ++;
			}

			void error(int line, int column, const std::string& message) {
				reports.emplace_back(Report::ERROR, line, column, message, unit);
				errors ++;
			}

			void link(asmio::BufferMarker marker, const std::string& message) {
				reports.emplace_back(Report::LINK, marker.section, marker.offset, message, unit);
				errors ++;
			}

		private:

			std::list<Report> reports;
			const std::string unit;
			const bool ansi;
			int errors, warnings;

	};

}