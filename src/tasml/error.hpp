#pragma once

#include "external.hpp"

struct ErrorHandler {

	public:

		class Report {

			public:

				enum Severity : uint32_t {
					WARNING,
					ERROR,
					FATAL
				};

				Report(Severity severity, uint32_t line, uint32_t column, const std::string& message, const std::string& unit)
				: line(line), column(column), severity(severity), message(message), unit(unit) {}

				const char* typestr(bool ansi) const {
					if (severity == WARNING) return ansi ? "\033[33;1mWarning:\033[0m" : "Warning:";
					if (severity == ERROR) return ansi ? "\033[31;1mError:\033[0m" : "Error:";
					if (severity == FATAL) return ansi ? "\033[31;1mFatal Error:\033[0m" : "Fatal Error:";

					return ansi ? "\033[31;1mInvalid:\033[0m" : "Invalid:";
				}

				void dump(bool ansi) const {
					printf("%s:%d %s %s!\n", unit.c_str(), line, typestr(ansi), message.c_str());
				}

			private:

				uint32_t line;
				uint32_t column;
				Severity severity;
				std::string message;
				std::string unit;

		};

		ErrorHandler(const std::string& unit, bool ansi)
		: unit(unit), ansi(ansi), errors(0), warnings(0) {}

		void dump() {
			for (Report report : reports) {
				report.dump(ansi);
			}

			reports.clear();
			warnings = 0;
			errors = 0;
		}

		bool ok() const {
			return errors == 0;
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

		[[noreturn]]
		void fatal(int line, int column, const std::string& message) {
			reports.emplace_back(Report::FATAL, line, column, message, unit);
			errors ++;

			throw std::runtime_error {"Fatal error occurred!"};
		}

	private:

		std::list<Report> reports;
		const std::string unit;
		const bool ansi;

		int errors, warnings;

};