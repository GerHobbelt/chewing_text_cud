
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

static constexpr const char *testfilepath = "H:\\prj-tmp-chk\\tmp\\__sqlite-odbc-mud-etc.dirlist.txt";






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

BENCHMARK_F(SplitFileContentsFixture, BM_WithFixture)(benchmark::State& state) {
	size_t size_4_stats = 0;
	size_t items_4_stats = 0;

	//while (state.KeepRunning()) {
	for (auto _ : state) {
		FileContentProcessingOptions proc_opts = {
			.mode = FileContentProcessingOptions::ParseMode::ToTextLines,

			.trim_outer_whitespace = true,
			.accept_comment_lines = true
		};

		ExtendedFileContent rv(this->data);
		std::error_code ec;
		rv.parseContentAsLines(proc_opts, ec);
		assert(!ec);
		assert(rv.lines.size() > 10);

		items_4_stats = rv.lines.size();
		size_4_stats = this->data.content_size();
	}

	state.SetBytesProcessed(state.iterations() * size_4_stats);
	state.SetItemsProcessed(state.iterations() * items_4_stats);
}

BENCHMARK_REGISTER_F(SplitFileContentsFixture, BM_WithFixture);

