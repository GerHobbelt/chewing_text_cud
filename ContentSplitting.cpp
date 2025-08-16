
#include "ContentSplitting.hpp"

#include "ResponseFileHandling.hpp"
#include "ReadFileContents.hpp"
#include "PrivateUtilities.hpp"

#include <string.h>


namespace text_processing {

	namespace fs = std::filesystem;

	// -----------------------------------------------------------------------------------------

	static inline bool is_blank(const char c) {
		return c == ' ' || c == '\t';
	}

	static inline bool is_eolz(const char c) {
		return c == '\r' || c == '\n' || c == '\0';
	}

	static inline bool is_eol_or_blank(const char c) {
		return c == '\r' || c == '\n' || c == ' ' || c == '\t';
	}

	void ExtendedFileContent::parseContentAsLines(const FileContentProcessingOptions& options, std::error_code &ec) {
		ec.clear();

		std::string_view d = file_content.content_view();

		// apply heuristic to estimate the number of lines that will be found
		lines.reserve(d.size() / 10);

		// scan the response file:
		size_t failure_count = 0;
		const auto* ptr = d.data();
		for (size_t i = 0, l = d.size(); i < l; i++) {
			while (is_blank(ptr[i])) {
				i++;
			}
			switch (ptr[i]) {
			case '\r':
			case '\n':
			case 0:
				// end-of-line ~ empty line. ignore.
				continue;

			case '#':
				if (options.accept_comment_lines) {
					while (!is_eolz(ptr[i])) {
						i++;
					}
					continue;
				}
				//[[fallthrough]]
			default:
				auto start = i;
				// path MAY have INTERNAL whitespace:
				while (!is_eolz(ptr[i])) {
					i++;
				}
				const auto ei = i;
				i--;
				// but trim off trailing whitespace!
				while (is_blank(ptr[i])) {
					i--;
				}
				i++;

				std::string_view line(ptr + start, i - start);
				assert(!line.empty());

				lines.push_back(line);

				// as we are not interested in empty lines anyway, we can skip EOL chars until we hit the next non-empty line:
				// this saves us several rounds through the main switch/case loop.
				//
				// continue beyond the trailing whitespace: no need to scan that stuff once again.
				i = ei;
				while (is_eol_or_blank(ptr[i])) {
					i++;
				}
				continue;
			}
		}
	}

	void ExtendedFileContent::parseContentAsParagraphs(const FileContentProcessingOptions& options, std::error_code &ec) {
		ec.clear();

		std::string_view d = file_content.content_view();

		// apply heuristic to estimate the number of lines that will be found
		lines.reserve(d.size() / 10);

		// scan the response file:
		size_t failure_count = 0;
		const auto* ptr = d.data();
		for (size_t i = 0, l = d.size(); i < l; i++) {
			while (is_blank(ptr[i])) {
				i++;
			}
			switch (ptr[i]) {
			case '\r':
			case '\n':
			case 0:
				// end-of-line ~ empty line. ignore.
				continue;

			case '#':
				if (options.accept_comment_lines) {
					while (!is_eolz(ptr[i])) {
						i++;
					}
					continue;
				}
				//[[fallthrough]]
			default:
				auto start = i;
				// path MAY have INTERNAL whitespace:
				while (!is_eolz(ptr[i])) {
					i++;
				}
				i--;
				// but trim off trailing whitespace!
				while (is_blank(ptr[i])) {
					i--;
				}
				i++;

				std::string_view line(ptr + start, i - start);
				assert(!line.empty());

				lines.push_back(line);
				continue;
			}
		}
	}

	void ExtendedFileContent::parseContentAsWords(const FileContentProcessingOptions& options, std::error_code &ec) {
		ec.clear();
		return;
	}

	void ExtendedFileContent::parseContentAsNGrams(const FileContentProcessingOptions& options, std::error_code &ec) {
		ec.clear();
		return;
	}

}

