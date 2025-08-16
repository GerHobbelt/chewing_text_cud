/*
2025-08-15T23:30:35+02:00
Running Z:\lib\tooling\qiqqa\MuPDF\platform\win32\bin\Debug-Unicode-64bit-x64\libchewing_text_cud_benchmarks.exe
Run on (16 X 3593 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 16384 KiB (x2)
***WARNING*** Library was built as DEBUG. Timings may be affected.
--------------------------------------------------------------------------------------------------
Benchmark                                        Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------------------
SplitFileContentsFixture/BM_WithFixture  645067100 ns    656250000 ns            1 bytes_per_second=224.973Mi/s items_per_second=2.27852M/s
SplitFileContentsFixture/BM_WithFixture  619888600 ns    625000000 ns            1 bytes_per_second=236.222Mi/s items_per_second=2.39245M/s
BM_ReadFileContents_Style_8               83060700 ns     83333333 ns            9 bytes_per_second=1.74685Gi/s
BM_ReadFileContents_Style_1              519804000 ns    515625000 ns            1 bytes_per_second=286.33Mi/s
BM_ReadFileContents_Style_2             7727849300 ns   7718750000 ns            1 bytes_per_second=19.1273Mi/s
BM_ReadFileContents_Style_3             4693152900 ns   4703125000 ns            1 bytes_per_second=31.3916Mi/s
BM_ReadFileContents_Style_4              471184350 ns    468750000 ns            2 bytes_per_second=314.963Mi/s
BM_ReadFileContents_Style_5              372433500 ns    367187500 ns            2 bytes_per_second=402.08Mi/s
BM_ReadFileContents_Style_6              355662900 ns    351562500 ns            2 bytes_per_second=419.951Mi/s
BM_ReadFileContents_Style_7               75576200 ns     76388889 ns            9 bytes_per_second=1.90566Gi/s
BM_ReadFileContents_Style_8               81855767 ns     81597222 ns            9 bytes_per_second=1.78402Gi/s

Release Win64 build:

2025-08-15T23:34:22+02:00
Running Z:\lib\tooling\qiqqa\MuPDF\platform\win32\bin\Release-Unicode-64bit-x64\libchewing_text_cud_benchmarks.exe
Run on (16 X 3593 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 16384 KiB (x2)
--------------------------------------------------------------------------------------------------
Benchmark                                        Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------------------
SplitFileContentsFixture/BM_WithFixture  144042780 ns    143750000 ns            5 bytes_per_second=1.00298Gi/s items_per_second=10.402M/s
SplitFileContentsFixture/BM_WithFixture  155262020 ns    156250000 ns            5 bytes_per_second=944.889Mi/s items_per_second=9.5698M/s
BM_ReadFileContents_Style_8               71439064 ns     71022727 ns           11 bytes_per_second=2.04964Gi/s
BM_ReadFileContents_Style_1              331337950 ns    312500000 ns            2 bytes_per_second=472.444Mi/s
BM_ReadFileContents_Style_2             1454917200 ns   1437500000 ns            1 bytes_per_second=102.705Mi/s
BM_ReadFileContents_Style_3             1335120800 ns   1343750000 ns            1 bytes_per_second=109.871Mi/s
BM_ReadFileContents_Style_4              206484233 ns    203125000 ns            3 bytes_per_second=726.837Mi/s
BM_ReadFileContents_Style_5              181061600 ns    183593750 ns            4 bytes_per_second=804.161Mi/s
BM_ReadFileContents_Style_6              173467975 ns    171875000 ns            4 bytes_per_second=858.99Mi/s
BM_ReadFileContents_Style_7               67337527 ns     68181818 ns           11 bytes_per_second=2.13504Gi/s
BM_ReadFileContents_Style_8               68345136 ns     68181818 ns           11 bytes_per_second=2.13504Gi/s

after patching FileReader to using C API fopen/fread/fclose in binary mode:

2025-08-16T00:03:46+02:00
Running Z:\lib\tooling\qiqqa\MuPDF\platform\win32\bin\Release-Unicode-64bit-x64\libchewing_text_cud_benchmarks.exe
Run on (16 X 3593 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 16384 KiB (x2)
--------------------------------------------------------------------------------------------------
Benchmark                                        Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------------------
SplitFileContentsFixture/BM_WithFixture  155399425 ns    156250000 ns            4 bytes_per_second=954.015Mi/s items_per_second=9.5698M/s
SplitFileContentsFixture/BM_WithFixture  154769125 ns    156250000 ns            4 bytes_per_second=954.015Mi/s items_per_second=9.5698M/s
BM_ReadFileContents_Style_8               71106336 ns     71022727 ns           11 bytes_per_second=2.04964Gi/s
BM_ReadFileContents_Style_1              306478650 ns    304687500 ns            2 bytes_per_second=484.558Mi/s
BM_ReadFileContents_Style_2             1301225800 ns   1312500000 ns            1 bytes_per_second=112.487Mi/s
BM_ReadFileContents_Style_3             1269804700 ns   1265625000 ns            1 bytes_per_second=116.653Mi/s
BM_ReadFileContents_Style_4              118435667 ns    119791667 ns            6 bytes_per_second=1.2152Gi/s
BM_ReadFileContents_Style_5              166565625 ns    167968750 ns            4 bytes_per_second=878.966Mi/s
BM_ReadFileContents_Style_6              152973920 ns    153125000 ns            5 bytes_per_second=964.172Mi/s
BM_ReadFileContents_Style_7               70815373 ns     71022727 ns           11 bytes_per_second=2.04964Gi/s
BM_ReadFileContents_Style_8               69748820 ns     70312500 ns           10 bytes_per_second=2.07035Gi/s

*/

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
#include <windows.h>

#undef min
#undef max
#endif

#define BENCHMARK_FAMILY_ID     "chewing_text_cud"

#include <benchmark/benchmark.h>


using namespace text_processing;

namespace fs = std::filesystem;

static constexpr const char *testfilepath = "H:\\prj-tmp-chk\\tmp\\__sqlite-odbc-mud-etc.dirlist.txt";

// nearly the fastest; the only drawback (IMO) is the slightly iffy way we need to deal
// with those istream status flags, expecting the fail bit to be set when reading text files on Windows in non-binary mode.
// The way that is resolved here is by querying the `gcount()` value on EOF and *assuming* an OKAY read when that
// value is half the filesize or better.
static void BM_ReadFileContents_Style_1(benchmark::State& state) {
	size_t size_4_stats = 0;

	for (auto _ : state) {
		auto fspec = locateFile(testfilepath);
		assert(fspec.has_value());
		path filepath = fspec.value();

		// https://medium.com/@nerudaj/tuesday-coding-tip-78-many-ways-of-reading-a-file-in-c-e66191dc60e3

		std::error_code ec;
		if (const std::uintmax_t filesize = fs::file_size(filepath, ec); ec) {
			LIBASSERT_UNREACHABLE(std::format("file size for file \"{}\" cannot be determined; {}", filepath.generic_string(), ec.message()));
		} else {
			if (false) std::cout << filepath.generic_string() << " size = " << HumanReadable{filesize} << '\n';

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
			assert(size_read >= filesize / 2); // account for CRLF conversion: any gcount() value at or above this number is deemed OK.

			size_4_stats = content.length();

			assert(content.size() > 1000);
		}
	}

	state.SetBytesProcessed(state.iterations() * size_4_stats);
}


// 20x slower than the best one. iterators... a nice idea but still crappy 25+ years after their invention in the STL.
static void BM_ReadFileContents_Style_2(benchmark::State& state) {
	size_t size_4_stats = 0;

	for (auto _ : state) {
		auto fspec = locateFile(testfilepath);
		assert(fspec.has_value());
		path filepath = fspec.value();

		// https://medium.com/@nerudaj/tuesday-coding-tip-78-many-ways-of-reading-a-file-in-c-e66191dc60e3

		std::error_code ec;
		if (const std::uintmax_t filesize = fs::file_size(filepath, ec); ec) {
			LIBASSERT_UNREACHABLE(std::format("file size for file \"{}\" cannot be determined; {}", filepath.generic_string(), ec.message()));
		} else {
			if (false) std::cout << filepath.generic_string() << " size = " << HumanReadable{filesize} << '\n';

			auto&& stream = std::ifstream(filepath);
			std::string content = std::string(
				std::istreambuf_iterator<char>(stream),
				std::istreambuf_iterator<char>());

			size_4_stats = content.length();

			assert(content.size() > 1000);
		}
	}

	state.SetBytesProcessed(state.iterations() * size_4_stats);
}


// 10x slower than the fastest one (#4); it's cute code, dealing with the rdbuf(), but iostreams can be slow indeed.
// Though DO NOTE that #1 also uses istream, but travels through the read() API which turns out to be fast (nearly as fast as the fastest #4).
static void BM_ReadFileContents_Style_3(benchmark::State& state) {
	size_t size_4_stats = 0;

	for (auto _ : state) {
		auto fspec = locateFile(testfilepath);
		assert(fspec.has_value());
		path filepath = fspec.value();

		// https://medium.com/@nerudaj/tuesday-coding-tip-78-many-ways-of-reading-a-file-in-c-e66191dc60e3

		std::error_code ec;
		if (const std::uintmax_t filesize = fs::file_size(filepath, ec); ec) {
			LIBASSERT_UNREACHABLE(std::format("file size for file \"{}\" cannot be determined; {}", filepath.generic_string(), ec.message()));
		} else {
			if (false) std::cout << filepath.generic_string() << " size = " << HumanReadable{filesize} << '\n';

			auto&& stream = std::ifstream(filepath);
			auto&& buffer = std::stringstream();
			buffer << stream.rdbuf();
			std::string content = buffer.str();

			size_4_stats = content.length();

			assert(content.size() > 1000);
		}
	}

	state.SetBytesProcessed(state.iterations() * size_4_stats);
}


// fastest kid on the block; using low level APIs to pump the goods into a buffer, pronto.
static void BM_ReadFileContents_Style_4(benchmark::State& state) {
	size_t size_4_stats = 0;

	for (auto _ : state) {
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

			size_4_stats = v.length();

			std::string content{v};

			assert(content.size() > 1000);
		}
	}

	state.SetBytesProcessed(state.iterations() * size_4_stats);
}


// just for the kicks: testing the standard C FILE API as well, just to see where it lands...
//
// WHOA! What just happened?! #5 is FASTER than #4 (low level I/O) @ 38/44 ratio!
static void BM_ReadFileContents_Style_5(benchmark::State& state) {
	size_t size_4_stats = 0;

	for (auto _ : state) {
		auto fspec = locateFile(testfilepath);
		assert(fspec.has_value());
		path filepath = fspec.value();

		// https://medium.com/@nerudaj/tuesday-coding-tip-78-many-ways-of-reading-a-file-in-c-e66191dc60e3

		std::error_code ec;
		if (const std::uintmax_t filesize = fs::file_size(filepath, ec); ec) {
			LIBASSERT_UNREACHABLE(std::format("file size for file \"{}\" cannot be determined; {}", filepath.generic_string(), ec.message()));
		} else {
			if (false) std::cout << filepath.generic_string() << " size = " << HumanReadable{filesize} << '\n';

			std::string content;
			content.resize(filesize);

			FILE *f = fopen(reinterpret_cast<const char *>(filepath.generic_u8string().c_str()), "r");
			auto len = fread(content.data(), 1, filesize, f);
			assert(feof(f));
			// we're reading a file in text mode, so filesize != len
			assert(len > 0);
			auto size_read = len;
			content.resize(size_read);
			assert(size_read >= filesize / 2); // account for CRLF conversion: any gcount() value at or above this number is deemed OK.
			fclose(f);
			f = nullptr;

			size_4_stats = content.length();

			assert(content.size() > 1000);
		}
	}

	state.SetBytesProcessed(state.iterations() * size_4_stats);
}

// C FILE API, text mode, using TextBuffer instead of std::string:
static void BM_ReadFileContents_Style_6(benchmark::State& state) {
	size_t size_4_stats = 0;

	for (auto _ : state) {
		auto fspec = locateFile(testfilepath);
		assert(fspec.has_value());
		path filepath = fspec.value();

		// https://medium.com/@nerudaj/tuesday-coding-tip-78-many-ways-of-reading-a-file-in-c-e66191dc60e3

		std::error_code ec;
		if (const std::uintmax_t filesize = fs::file_size(filepath, ec); ec) {
			LIBASSERT_UNREACHABLE(std::format("file size for file \"{}\" cannot be determined; {}", filepath.generic_string(), ec.message()));
		} else {
			if (false) std::cout << filepath.generic_string() << " size = " << HumanReadable{filesize} << '\n';

			TextBuffer content(filesize + TextBuffer::sentinel_size);

			FILE *f = fopen(reinterpret_cast<const char *>(filepath.generic_u8string().c_str()), "r");
			auto len = fread(content.data(), 1, filesize, f);
			assert(feof(f));
			// we're reading a file in text mode, so filesize != len
			assert(len > 0);
			auto size_read = len;
			content.set_content_size(size_read);
			assert(size_read >= filesize / 2); // account for CRLF conversion: any gcount() value at or above this number is deemed OK.
			fclose(f);
			f = nullptr;

			size_4_stats = content.content_size();

			assert(content.content_size() > 1000);
		}
	}

	state.SetBytesProcessed(state.iterations() * size_4_stats);
}

// C FILE API, BINARY mode, using TextBuffer instead of std::string:
//
// --> 5x (!sic!) FASTER than the fastest text-based I/O run above.
// 
// IFF we can split lines with less than 5x cost down-range (line splitting, etc.)
// we clearly have the ultimate winner here.
// 
// C++ zealots may frown upon the use of the C FILE API, but that one is the fastest
// of the bunch, including low level io.h I/O.
// 
// Hm, PROBABLY *native* Win32 APIs are faster still as that's what underlays all the others...
static void BM_ReadFileContents_Style_7(benchmark::State& state) {
	size_t size_4_stats = 0;

	for (auto _ : state) {
		auto fspec = locateFile(testfilepath);
		assert(fspec.has_value());
		path filepath = fspec.value();

		// https://medium.com/@nerudaj/tuesday-coding-tip-78-many-ways-of-reading-a-file-in-c-e66191dc60e3

		std::error_code ec;
		if (const std::uintmax_t filesize = fs::file_size(filepath, ec); ec) {
			LIBASSERT_UNREACHABLE(std::format("file size for file \"{}\" cannot be determined; {}", filepath.generic_string(), ec.message()));
		} else {
			if (false) std::cout << filepath.generic_string() << " size = " << HumanReadable{filesize} << '\n';

			TextBuffer content(filesize + TextBuffer::sentinel_size);

			FILE *f = fopen(reinterpret_cast<const char *>(filepath.generic_u8string().c_str()), "rb");
			auto len = fread(content.data(), 1, filesize + 1, f);
			assert(feof(f));
			// we're reading a file in text mode, so filesize != len
			assert(len > 0);
			assert(len == filesize);
			auto size_read = len;
			content.set_content_size(size_read);
			assert(size_read == filesize);
			fclose(f);
			f = nullptr;

			size_4_stats = content.content_size();

			assert(content.content_size() > 1000);
		}
	}

	state.SetBytesProcessed(state.iterations() * size_4_stats);
}


#if defined(_WIN32)

// Win32 API, BINARY mode, using TextBuffer instead of std::string:
//
static void BM_ReadFileContents_Style_8(benchmark::State& state) {
	size_t size_4_stats = 0;

	for (auto _ : state) {
		auto fspec = locateFile(testfilepath);
		assert(fspec.has_value());
		path filepath = fspec.value();

		// https://medium.com/@nerudaj/tuesday-coding-tip-78-many-ways-of-reading-a-file-in-c-e66191dc60e3

		std::error_code ec;
		if (const std::uintmax_t filesize = fs::file_size(filepath, ec); ec) {
			LIBASSERT_UNREACHABLE(std::format("file size for file \"{}\" cannot be determined; {}", filepath.generic_string(), ec.message()));
		} else {
			if (false) std::cout << filepath.generic_string() << " size = " << HumanReadable{filesize} << '\n';

			TextBuffer content(filesize + TextBuffer::sentinel_size);

			HANDLE h = CreateFileW(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			assert(h != INVALID_HANDLE_VALUE);
			DWORD read_size = 0;
			auto ok = ReadFile(h, content.data(), filesize + 1, &read_size, nullptr);
			assert(ok != 0);
			assert(read_size == filesize);
			content.set_content_size(read_size);
			CloseHandle(h);
			h = NULL;

			size_4_stats = content.content_size();

			assert(content.content_size() > 1000);
		}
	}

	state.SetBytesProcessed(state.iterations() * size_4_stats);
}

#endif // defined(_WIN32)




#if defined(_WIN32)
BENCHMARK(BM_ReadFileContents_Style_8);
#endif
BENCHMARK(BM_ReadFileContents_Style_1);
BENCHMARK(BM_ReadFileContents_Style_2);
BENCHMARK(BM_ReadFileContents_Style_3);
BENCHMARK(BM_ReadFileContents_Style_4);
BENCHMARK(BM_ReadFileContents_Style_5);
BENCHMARK(BM_ReadFileContents_Style_6);
BENCHMARK(BM_ReadFileContents_Style_7);
#if defined(_WIN32)
BENCHMARK(BM_ReadFileContents_Style_8);
#endif



// Example usage:

#if 0

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

#endif

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

