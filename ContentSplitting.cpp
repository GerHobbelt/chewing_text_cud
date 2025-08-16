
#include "ContentSplitting.hpp"

#include "ResponseFileHandling.hpp"
#include "ReadFileContents.hpp"
#include "PrivateUtilities.hpp"

#include "PrivateIntrinsics.hpp"

#include <string.h>


namespace text_processing {

	namespace fs = std::filesystem;

	// -----------------------------------------------------------------------------------------

	static inline bool is_blank(const char c) {
		return c == ' ' || c == '\t';
	}

#if 0
	static inline bool is_eolz(const char c) {
		return !!memchr("\r\n", c, 3 /* including NUL */ );
	}

	static inline bool is_eol_or_blank(const char c) {
		return !!memchr("\r\n \t", c, 4);
	}
#else
	static inline bool is_eolz(const char c) {
		return c == '\r' || c == '\n' || c == '\0';
	}

	static inline bool is_eol_or_blank(const char c) {
		return c == '\r' || c == '\n' || c == ' ' || c == '\t';
	}
#endif

#if 0
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
				// path MAY have INTERNAL whitespace: find the terminating CR/LF/NUL
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
#else
	// ~10% faster.

	void ExtendedFileContent::parseContentAsLines(const FileContentProcessingOptions& options, std::error_code &ec) {
		ec.clear();

		// prep the actions table
		enum Action: uint8_t {
			noAction = 0,
			MarkEndOfLine,
			SkipWhitespace,
			SkipCommentLine,
		};
		Action actions[256] = {MarkEndOfLine, noAction};
		actions['\t'] = SkipWhitespace;
		actions['\r'] = MarkEndOfLine;
		actions['\n'] = MarkEndOfLine;
		actions['\v'] = SkipWhitespace;
		if (options.accept_comment_lines) {
			actions['#'] = SkipCommentLine;
		}

		std::string_view d = file_content.content_view();
		file_content.write_buffer_edge_sentinel();

		// apply heuristic to estimate the number of lines that will be found
		lines.reserve(d.size() / 10);

		// scan the response file:
		size_t failure_count = 0;
		const auto* ptr = d.data();
		for (size_t i = 0, l = d.size(); i < l; ) {
			uint8_t c = ptr[i++];
			switch (actions[c]) {
			case SkipWhitespace:
				while (is_blank(ptr[i])) {
					i--;
				}
				continue;

			case MarkEndOfLine:
				// end-of-line ~ empty line. ignore.
				continue;

			case SkipCommentLine:
				while (actions[ptr[i++]] != MarkEndOfLine) {
					;
				}
				continue;

			case noAction:
			default:
				auto start = i - 1;
				// path MAY have INTERNAL whitespace: find the terminating CR/LF/NUL
				while (actions[ptr[i++]] != MarkEndOfLine) {
					;
				}
				const auto ei = i;
				// but trim off trailing whitespace!
				while (actions[ptr[--i]] == SkipWhitespace) {
					;
				}
				i++;

				std::string_view line(ptr + start, i - start);
				assert(!line.empty());

				lines.push_back(line);

				// small aid for CRLF line terminations in files: ptr[i-1] is probably the CR, so we might speed things up
				// by quickly checking if ptr[i] is a LF:
				i = ei;
				if (actions[ptr[i]] == MarkEndOfLine) {
					++i;
				}
				continue;
			}
		}
	}
#endif

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

