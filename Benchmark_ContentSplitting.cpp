
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

static constexpr const char *testfilepath = "H:\\prj-tmp-chk\\tmp\\__scratch.dirlist.txt";






class SplitFileContentsFixture: public ::benchmark::Fixture {
protected:
	TextBuffer data;

public:
	void SetUp(benchmark::State& state) override {
		auto fspec = locateFile(testfilepath);
		assert(fspec.has_value());
		path filepath = fspec.value();

		// https://medium.com/@nerudaj/tuesday-coding-tip-78-many-ways-of-reading-a-file-in-c-e66191dc60e3

		std::error_code ec;
		if (const std::uintmax_t filesize = fs::file_size(filepath, ec); ec) {
			LIBASSERT_UNREACHABLE(std::format("file size for file \"{}\" cannot be determined; {}", filepath.generic_string(), ec.message()));
		} else {
			std::cout << filepath.generic_string() << " size = " << HumanReadable{filesize} << '\n';

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

	void TearDown(benchmark::State& state) override {
		data.clear();
	}

	~SplitFileContentsFixture() override {
		data.clear();
	}
};

BENCHMARK_F(SplitFileContentsFixture, BM_WithFixture)(benchmark::State& state) {
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
}

BENCHMARK_REGISTER_F(SplitFileContentsFixture, BM_WithFixture);

