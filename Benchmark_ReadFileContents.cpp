
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


static void BM_ReadFileContents_Style_1(benchmark::State& state) {
	auto fspec = locateFile(testfilepath);
	assert(fspec.has_value());
	path filepath = fspec.value();

	// https://medium.com/@nerudaj/tuesday-coding-tip-78-many-ways-of-reading-a-file-in-c-e66191dc60e3

	std::error_code ec;
	if (const std::uintmax_t filesize = fs::file_size(filepath, ec); ec) {
		LIBASSERT_UNREACHABLE(std::format("file size for file \"{}\" cannot be determined; {}", filepath.generic_string(), ec.message()));
	}
	else {
		std::cout << filepath.generic_string() << " size = " << HumanReadable{filesize} << '\n';

		//auto&& content = std::string(filesize, '\0');
		std::string content;
		//content.reserve(filesize);
		content.resize(filesize);

		auto&& stream = std::ifstream(filepath);
		stream.read(content.data(), filesize);
		assert(stream.eof());
		// failbit MAY be set: we're reading a file in text mode, so filesize != size_read
		auto size_read = stream.gcount();
		content.resize(size_read);

		assert(content.size() > 1000);
	}
}


static void BM_ReadFileContents_Style_2(benchmark::State& state) {
	auto fspec = locateFile(testfilepath);
	assert(fspec.has_value());
	path filepath = fspec.value();

	// https://medium.com/@nerudaj/tuesday-coding-tip-78-many-ways-of-reading-a-file-in-c-e66191dc60e3

	std::error_code ec;
	if (const std::uintmax_t filesize = fs::file_size(filepath, ec); ec) {
		LIBASSERT_UNREACHABLE(std::format("file size for file \"{}\" cannot be determined; {}", filepath.generic_string(), ec.message()));
	} else {
		std::cout << filepath.generic_string() << " size = " << HumanReadable{filesize} << '\n';

		auto&& stream = std::ifstream(filepath);
		std::string content = std::string(
			std::istreambuf_iterator<char>(stream),
			std::istreambuf_iterator<char>());

		assert(content.size() > 1000);
	}
}



static void BM_ReadFileContents_Style_3(benchmark::State& state) {
	auto fspec = locateFile(testfilepath);
	assert(fspec.has_value());
	path filepath = fspec.value();

	// https://medium.com/@nerudaj/tuesday-coding-tip-78-many-ways-of-reading-a-file-in-c-e66191dc60e3

	std::error_code ec;
	if (const std::uintmax_t filesize = fs::file_size(filepath, ec); ec) {
		LIBASSERT_UNREACHABLE(std::format("file size for file \"{}\" cannot be determined; {}", filepath.generic_string(), ec.message()));
	} else {
		std::cout << filepath.generic_string() << " size = " << HumanReadable{filesize} << '\n';

		auto&& stream = std::ifstream(filepath);
		auto&& buffer = std::stringstream();
		buffer << stream.rdbuf();
		std::string content = buffer.str();

		assert(content.size() > 1000);
	}
}



static void BM_ReadFileContents_Style_4(benchmark::State& state) {
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

		std::string content{v};

		assert(content.size() > 1000);
	}
}

BENCHMARK(BM_ReadFileContents_Style_1);
BENCHMARK(BM_ReadFileContents_Style_2);
BENCHMARK(BM_ReadFileContents_Style_3);
BENCHMARK(BM_ReadFileContents_Style_4);



// Example usage:

// Define a function that executes the code to be measured a
// specified number of times:
static void BM_StringCreation(benchmark::State& state) {
  for (auto _ : state)
	std::string empty_string;
}

// Register the function as a benchmark
BENCHMARK(BM_StringCreation);

// Define another benchmark
static void BM_StringCopy(benchmark::State& state) {
  std::string x = "hello";
  for (auto _ : state)
	std::string copy(x);
}
BENCHMARK(BM_StringCopy);

// Sometimes a family of microbenchmarks can be implemented with
// just one routine that takes an extra argument to specify which
// one of the family of benchmarks to run.  For example, the following
// code defines a family of microbenchmarks for measuring the speed
// of memcpy() calls of different lengths:

static void BM_memcpy(benchmark::State& state) {
  char* src = new char[state.range(0)]; char* dst = new char[state.range(0)];
  memset(src, 'x', state.range(0));
  for (auto _ : state)
	memcpy(dst, src, state.range(0));
  state.SetBytesProcessed(state.iterations() * state.range(0));
  delete[] src; delete[] dst;
}
BENCHMARK(BM_memcpy)->Arg(8)->Arg(64)->Arg(512)->Arg(1<<10)->Arg(8<<10);

// The preceding code is quite repetitive, and can be replaced with the
// following short-hand.  The following invocation will pick a few
// appropriate arguments in the specified range and will generate a
// microbenchmark for each such argument.
BENCHMARK(BM_memcpy)->Range(8, 8<<10);

#if 0

// You might have a microbenchmark that depends on two inputs.  For
// example, the following code defines a family of microbenchmarks for
// measuring the speed of set insertion.
static void BM_SetInsert(benchmark::State& state) {
  std::set<int> data;
  for (auto _ : state) {
	state.PauseTiming();
	data = ConstructRandomSet(state.range(0));
	state.ResumeTiming();
	for (int j = 0; j < state.range(1); ++j)
	  data.insert(RandomNumber());
  }
}
BENCHMARK(BM_SetInsert)
   ->Args({1<<10, 128})
   ->Args({2<<10, 128})
   ->Args({4<<10, 128})
   ->Args({8<<10, 128})
   ->Args({1<<10, 512})
   ->Args({2<<10, 512})
   ->Args({4<<10, 512})
   ->Args({8<<10, 512});

// The preceding code is quite repetitive, and can be replaced with
// the following short-hand.  The following macro will pick a few
// appropriate arguments in the product of the two specified ranges
// and will generate a microbenchmark for each such pair.
BENCHMARK(BM_SetInsert)->Ranges({{1<<10, 8<<10}, {128, 512}});

// For more complex patterns of inputs, passing a custom function
// to Apply allows programmatic specification of an
// arbitrary set of arguments to run the microbenchmark on.
// The following example enumerates a dense range on
// one parameter, and a sparse range on the second.
static void CustomArguments(benchmark::internal::Benchmark* b) {
  for (int i = 0; i <= 10; ++i)
	for (int j = 32; j <= 1024*1024; j *= 8)
	  b->Args({i, j});
}
BENCHMARK(BM_SetInsert)->Apply(CustomArguments);

#endif

#if 0

// Templated microbenchmarks work the same way:
// Produce then consume 'size' messages 'iters' times
// Measures throughput in the absence of multiprogramming.
template <class Q> int BM_Sequential(benchmark::State& state) {
  Q q;
  typename Q::value_type v;
  for (auto _ : state) {
	for (int i = state.range(0); i--; )
	  q.push(v);
	for (int e = state.range(0); e--; )
	  q.Wait(&v);
  }
  // actually messages, not bytes:
  state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK_TEMPLATE(BM_Sequential, WaitQueue<int>)->Range(1<<0, 1<<10);

#endif

#if 0

// Use `Benchmark::MinTime(double t)` to set the minimum time used to determine how
// long to run the benchmark. This option overrides the `benchmark_min_time` flag.
//
// If a benchmark measures time manually, use `Benchmark::MinRelAccuracy(double r)`
// to set the required minimum relative accuracy used to determine how long to run
// the benchmark. This option overrides the `benchmark_min_rel_accuracy` flag.

void BM_test(benchmark::State& state) {
 //... body ...
}
BENCHMARK(BM_test)->MinTime(2.0); // Run for at least 2 seconds.

#endif



extern "C"
BENCHMARK_EXPORT int main(int, const char**);

#if 01

// Augment the main() program to invoke benchmarks if specified
// via the --benchmark_filter command line flag.  E.g.,
//       my_unittest --benchmark_filter=all
//       my_unittest --benchmark_filter=BM_StringCreation
//       my_unittest --benchmark_filter=String
//       my_unittest --benchmark_filter='Copy|Creation'
BENCHMARK_EXPORT int main(int argc, const char** argv) {
	benchmark::MaybeReenterWithoutASLR(argc, argv);
	benchmark::Initialize(&argc, argv);
	if (::benchmark::ReportUnrecognizedArguments(argc, argv))
		return 1; 
	benchmark::RunSpecifiedBenchmarks(BENCHMARK_FAMILY_ID, false);
	benchmark::Shutdown();
	return 0;
}

#else

BENCHMARK_MAIN();

#endif

