
#include "ContentSplitting.hpp"
#include "ResponseFileHandling.hpp"
#include "ReadFileContents.hpp"
#include "PrivateUtilities.hpp"

#include <libassert/assert.h>
#include <cassert>

#if defined(_WIN32)
#ifndef _CRT_DECLARE_NONSTDC_NAMES
#define _CRT_DECLARE_NONSTDC_NAMES  1
#endif
#include <io.h>
#include <fcntl.h>
#include <string.h>
#endif

#define BENCHMARK_FAMILY_ID     "chewing_text_cud"

#include <benchmark/benchmark.h>


using namespace text_processing;

namespace fs = std::filesystem;

static constexpr const char *testfilepath = "H:\\prj-tmp-chk\\tmp\\__sqlite-odbc-mud-etc.dirlist.txt"; // "___etc.dirlist.txt"; // "__scratch.dirlist.txt"; //"__sqlite-odbc-mud-etc.dirlist.txt";






class SplitFileContentsFixture: public ::benchmark::Fixture {
protected:
	TextBuffer data;
	size_t setup_count = 0;

public:
	SplitFileContentsFixture(): Fixture() {
		// fixture constructor is invoked multiple times (2x) before first test is run;
		// to reduce costs, we only do a swift 'mark/unmark' in the constructor/destructor
		// and delay the big prep work until the first actual setup round and last teardown round.
		setup_count = 0;
	}

	void SetUp(benchmark::State& state) override {
		// costly: invoked for each run/round; we're doing the heavy prep lifting in the constructo+destructor instead!
		++setup_count;
		if (setup_count == 1) {
			auto fspec = locateFile(testfilepath);
			assert(fspec.has_value());
			path filepath = fspec.value();

			// https://medium.com/@nerudaj/tuesday-coding-tip-78-many-ways-of-reading-a-file-in-c-e66191dc60e3

			std::error_code ec;
			if (const std::uintmax_t filesize = fs::file_size(filepath, ec); ec) {
				LIBASSERT_UNREACHABLE(std::format("file size for file \"{}\" cannot be determined; {}", filepath.generic_string(), ec.message()));
			} else {
				if (false) std::cout << filepath.generic_string() << " size = " << HumanReadable{filesize} << '\n';

				FileReader reader;
				auto o = reader.open(filepath);
				if (o) {
					LIBASSERT_UNREACHABLE(std::format("error processing file \"{}\": error {}:{}", filepath.generic_string(), int(o.value().code), o.value().message));
				}
				auto r = reader.readAllContent(filesize);
				if (!r.has_value()) {
					LIBASSERT_UNREACHABLE(std::format("error processing file \"{}\": error {}:{}", filepath.generic_string(), int(r.error().code), r.error().message));
				}
				std::string_view v = reader.data.content_view();
				reader.close();

				data.clear();
				data = v;

				assert(v.size() > 1000);
			}
		}
	}

	void TearDown(benchmark::State& state) override {
		// costly: invoked for each run/round; we're doing the heavy prep lifting in the constructo+destructor instead!
		if (setup_count == 1) {
			data.clear();
		}
		--setup_count;
	}

	~SplitFileContentsFixture() override {
		data.clear();
	}
};



BENCHMARK_F(SplitFileContentsFixture, BM_WithFixture_1)(benchmark::State& state) {
	size_t size_4_stats = 0;
	size_t items_4_stats = 0;

	//while (state.KeepRunning()) {
	for (auto _ : state) {
		FileContentProcessingOptions proc_opts = {
			.mode = FileContentProcessingOptions::ParseMode::ToTextLines,

			.trim_outer_whitespace = true,
			.accept_comment_lines = true
		};

		// preparation takes a while for very large input files...
		state.PauseTiming();
		ExtendedFileContent rv(this->data);
		state.ResumeTiming();

		std::error_code ec;
		rv.parseContentAsLines(proc_opts, ec);
		assert(!ec);
		assert(rv.lines.size() > 10);

		state.PauseTiming();
		items_4_stats = rv.lines.size();
		size_4_stats = this->data.content_size();
		state.ResumeTiming();
	}

	state.SetBytesProcessed(state.iterations() * size_4_stats);
	state.SetItemsProcessed(state.iterations() * items_4_stats);
}


#if 0   // BENCHMARK_F() already registers the benchmark method.
BENCHMARK_REGISTER_F(SplitFileContentsFixture, BM_WithFixture);
#endif


static inline bool is_blank(const char c) {
	return c == ' ' || c == '\t';
}

static inline bool is_eolz(const char c) {
	return c == '\r' || c == '\n' || c == '\0';
}

static inline bool is_eol_or_blank(const char c) {
	return c == '\r' || c == '\n' || c == ' ' || c == '\t';
}


// parseContentAsLines():
// 
// testing the first implementation of the line splitter.

BENCHMARK_F(SplitFileContentsFixture, BM_WithFixture_2)(benchmark::State& state) {
	size_t size_4_stats = 0;
	size_t items_4_stats = 0;

	//while (state.KeepRunning()) {
	for (auto _ : state) {
		FileContentProcessingOptions proc_opts = {
			.mode = FileContentProcessingOptions::ParseMode::ToTextLines,

			.trim_outer_whitespace = true,
			.accept_comment_lines = true
		};

		// preparation takes a while for very large input files...
		state.PauseTiming();
		ExtendedFileContent rv(this->data);
		state.ResumeTiming();

		std::error_code ec;
		//rv.parseContentAsLines(proc_opts, ec);


		//void ExtendedFileContent::parseContentAsLines(const FileContentProcessingOptions& options, std::error_code &ec)
		{
			ec.clear();

			std::string_view d = rv.file_content.content_view();
			rv.file_content.write_buffer_edge_sentinel();

			// apply heuristic to estimate the number of lines that will be found
			rv.lines.reserve(d.size() / 10);

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
					if (proc_opts.accept_comment_lines) {
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

					rv.lines.push_back(line);

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
		//void ExtendedFileContent::parseContentAsLines(const FileContentProcessingOptions& options, std::error_code &ec)


		assert(!ec);
		assert(rv.lines.size() > 10);

		state.PauseTiming();
		items_4_stats = rv.lines.size();
		size_4_stats = this->data.content_size();
		state.ResumeTiming();
	}

	state.SetBytesProcessed(state.iterations() * size_4_stats);
	state.SetItemsProcessed(state.iterations() * items_4_stats);
}




// parseContentAsLines():
// 
// a new implementation, after discovering the inner run-until-EOL loop takes up most of the time (~50%)
// 
// is this ~10% faster?
//
// Nope. > 25% faster! (for very large files: 3GB)
// 
// The inner run-until-EOL loop is still the Big One(tm) but it dropped to ~40% of the total cost of the code,
// while the outer loop has been somewhat simplified and should execute faster as well. --> it does!
//
// 
// POST MORTEM NOTES:
// smarter folks than me may be able to turn this thing into a SIMD vectorized splitter loop, but I have
// looked at StringZilla et al and they don't provide faster implementations for the critical functionality,
// which is equivalent to using strcspn/strspn as we scan through the content hunting for multiple specials
// (CR, LF, TAB, ...) at any time.
// The tightly packed actions[] lookup table was imagined as the best potential optimization approach here instead,
// producing a code with fewer conditions, hence less chance at prediction faults and better approaching
// branchless code.
//
// The stack-local and run-time generated lookup table was expected to be somewhat costly, but compared to the
// Textbuffer sizes we're processing in the benchmark, this is utterly negligible. Of course, this may be another
// story altogether in actually production, as we are expecting to process a lot of SMALL FILES as well!
// --> we might gain from a singleton action table generator or static table which is generated by external tools.
//
// HOWEVER, do note that the stack-local action state machine is INTENTIONAL as we wish to re-use this technology
// for our word + ngram scanners, which are expected to PATCH the action table at runtime to suit their local
// needs, i.e. a self-modifying state machine!
//

BENCHMARK_F(SplitFileContentsFixture, BM_WithFixture_3)(benchmark::State& state) {
	size_t size_4_stats = 0;
	size_t items_4_stats = 0;

	//while (state.KeepRunning()) {
	for (auto _ : state) {
		FileContentProcessingOptions proc_opts = {
			.mode = FileContentProcessingOptions::ParseMode::ToTextLines,

			.trim_outer_whitespace = true,
			.accept_comment_lines = true
		};

		// preparation takes a while for very large input files...
		state.PauseTiming();
		ExtendedFileContent rv(this->data);
		state.ResumeTiming();

		std::error_code ec;
		//rv.parseContentAsLines(proc_opts, ec);


		//void ExtendedFileContent::parseContentAsLines(const FileContentProcessingOptions& options, std::error_code &ec)
		{
			ec.clear();

			// prep the actions table
			enum Action: uint8_t {
				noAction = 0,
				MarkEndOfLine,
				SkipWhitespace,
				SkipCommentLine,
			};
			Action actions[256] = {MarkEndOfLine, noAction};
			if (proc_opts.trim_outer_whitespace) {
				actions['\t'] = SkipWhitespace;
				actions['\v'] = SkipWhitespace;
				actions[' '] = SkipWhitespace;
			}
			actions['\r'] = MarkEndOfLine;
			actions['\n'] = MarkEndOfLine;
			if (proc_opts.accept_comment_lines) {
				actions['#'] = SkipCommentLine;
			}

			std::string_view d = rv.file_content.content_view();
			rv.file_content.write_buffer_edge_sentinel();

			// apply heuristic to estimate the number of lines that will be found
			rv.lines.reserve(d.size() / 10);

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

				[[likely]] case noAction:
				default:
					auto start = i - 1;
					// path MAY have INTERNAL whitespace: find the terminating CR/LF/NUL
					while (actions[ptr[i++]] != MarkEndOfLine) {
						;
					}
					const auto ei = i;
					// but trim off trailing whitespace!
					// 
					//if (proc_opts.trim_outer_whitespace) {    --> only then does state `SkipWhitespace` exist in the actions table.
					while (actions[ptr[--i]] == SkipWhitespace) {
						;
					}
					i++;
					//}

					std::string_view line(ptr + start, i - start);
					assert(!line.empty());

					rv.lines.push_back(line);

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
		//void ExtendedFileContent::parseContentAsLines(const FileContentProcessingOptions& options, std::error_code &ec)


		assert(!ec);
		assert(rv.lines.size() > 10);

		state.PauseTiming();
		items_4_stats = rv.lines.size();
		size_4_stats = this->data.content_size();
		state.ResumeTiming();
	}

	state.SetBytesProcessed(state.iterations() * size_4_stats);
	state.SetItemsProcessed(state.iterations() * items_4_stats);
}


