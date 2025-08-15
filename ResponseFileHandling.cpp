//
// Load a text file and assume it's a response file, i.e. a file listing other files to load.
//
// A response file may contain zero or more relative/absolute paths, one per line,
// and `#` prefixed comment lines.
//
// Produce an error when the file content does not look like a response file, or when a certain
// percentage of the paths do not exist (this can be due to mistakenly treating a simple word dictionary
// as a response file and is the second failure mode here).
//

#include <expected>
#include <string>
#include <vector>
#include <filesystem>
#include <system_error>
#include <format>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
#if defined(_WIN32)
#ifndef _CRT_DECLARE_NONSTDC_NAMES
#define _CRT_DECLARE_NONSTDC_NAMES  1
#endif
#include <io.h>
#include <fcntl.h>
#include <string.h>
#endif


namespace text_processing {

	using std::filesystem::path;

	struct ResponsefileProcessingOptions {
		float tolerated_nonexist_ratio = 0.0f;

		std::vector<path> search_paths;

		bool accept_absolute_paths{true};
		bool accept_relative_paths{true};
		bool accept_comment_lines{true};

		bool specfile_path_is_also_search_path{true};
	};

	struct ErrorResponse {
		std::errc code;
		std::string message;
	};

	struct ResponseFilesSet {
		// stores the responsefile content
		std::string file_content;

		// a map of pointers into the response file content, each describing one target file path.
		typedef std::vector<std::string_view> list;

		list files;
	};

	using ResponseFileParseResult = std::expected<ResponseFilesSet, ErrorResponse>;

	ResponseFileParseResult processAsResponseFile(const path& filepath, const ResponsefileProcessingOptions &options = {});
	ResponseFileParseResult processAsResponseFile(const std::string &response_file_content, const path& filepath, const ResponsefileProcessingOptions &options = {});
	ResponseFileParseResult processAsResponseFile(std::string &&response_file_content, const path& filepath, const ResponsefileProcessingOptions &options = {});

	ResponseFileParseResult processAsResponseFile(ResponseFileParseResult &&data, const path& filepath, const ResponsefileProcessingOptions &options);

}



// --------------------------------------------------------------------

namespace text_processing {

	namespace fs = std::filesystem;

	// ripped from https://en.cppreference.com/w/cpp/filesystem/file_size.html
	struct HumanReadable
	{
		std::uintmax_t size{};

	private:
		friend std::ostream& operator<<(std::ostream& os, HumanReadable hr)
		{
			int o{};
			double mantissa = hr.size;
			for (; mantissa >= 1024.; mantissa /= 1024., ++o);
			os << std::ceil(mantissa * 10.) / 10. << "BKMGTPE"[o];
			return o ? os << "B (" << hr.size << ')' : os;
		}
	};

	std::expected<path, ErrorResponse> locateFile(const path &filepath, const path &source_filepath, const std::vector<path> &search_paths, bool specfile_path_is_also_search_path = false, bool accept_absolute_paths = true, bool accept_relative_paths = true) {
		if (filepath.is_relative()) {
			if (!accept_relative_paths) {
				return std::unexpected{ErrorResponse{std::errc::invalid_argument, std::format("relative paths, such as file \"{}\", are not accepted.", filepath.generic_string())}};
			}

			// apply search path:
			auto f = std::filesystem::canonical(filepath);
			if (std::filesystem::exists(f)) {
				return f;
			}

			if (specfile_path_is_also_search_path) {
				path f = source_filepath.parent_path();
				if (f == ".")
					f = fs::current_path();
				f /= filepath;
				f = std::filesystem::canonical(f);
				if (std::filesystem::exists(f)) {
					return f;
				}
			}

			for (int i = 0, l = search_paths.size(); i < l; i++) {
				path f = search_paths[i];
				if (f == ".")
					f = fs::current_path();
				f /= filepath;
				f = std::filesystem::canonical(f);
				if (std::filesystem::exists(f)) {
					return f;
				}
			}

			//ErrorResponse e{std::errc::no_such_file_or_directory, std::format("file \"{}\" does not exist.", filepath.generic_string())};
			//return std::unexpected{e};
			return std::unexpected{ErrorResponse{std::errc::no_such_file_or_directory, std::format("file \"{}\" does not exist.", filepath.generic_string())}};
		}
		// else: input is an absolute path, so no search_paths traversal or anything!
		if (!accept_absolute_paths) {
			return std::unexpected{ErrorResponse{std::errc::invalid_argument, std::format("absolute paths, such as file \"{}\", are not accepted.", filepath.generic_string())}};
		}

		path f = std::filesystem::canonical(filepath);
		if (std::filesystem::exists(filepath)) {
			return f;
		}

		//ErrorResponse e{std::errc::no_such_file_or_directory, std::format("file \"{}\" does not exist.", filepath.generic_string())};
		//return std::unexpected{e};
		return std::unexpected{ErrorResponse{std::errc::no_such_file_or_directory, std::format("file \"{}\" does not exist.", filepath.generic_string())}};
	}


	ResponseFileParseResult processAsResponseFile(const path &filepath, const ResponsefileProcessingOptions &options) {
		return locateFile(filepath, filepath, options.search_paths).and_then([options](path &&p) -> ResponseFileParseResult {
			// https://medium.com/@nerudaj/tuesday-coding-tip-78-many-ways-of-reading-a-file-in-c-e66191dc60e3

			std::error_code ec;
			if (const std::uintmax_t filesize = fs::file_size(p, ec); ec) {
				std::cout << p.generic_string() << " : " << ec.message() << '\n';
				return std::unexpected{ErrorResponse{std::errc::io_error, std::format("file size for file \"{}\" cannot be determined; {}", p.generic_string(), ec.message())}};
			}
			else {
				std::cout << p.generic_string() << " size = " << HumanReadable{filesize} << '\n';

#if 0
				//auto&& content = std::string(filesize, '\0');
				std::string content;
				//content.reserve(filesize);
				content.resize(filesize);

				auto&& stream = std::ifstream(p);
				stream.read(content.data(), filesize);
				assert(stream.eof());
				// failbit MAY be set: we're reading a file in text mode, so filesize != size_read
				auto size_read = stream.gcount();
				content.resize(size_read);

				ResponseFilesSet rv;
				rv.file_content = std::move(content);
				return processAsResponseFile(rv, p, options);
#elif 0
				auto&& stream = std::ifstream(p);
				std::string content(
					std::istreambuf_iterator<char>(stream),
					std::istreambuf_iterator<char>());

				ResponseFilesSet rv;
				rv.file_content = std::move(content);
				return processAsResponseFile(rv, p, options);
#elif 0
				auto&& stream = std::ifstream(p);
				auto&& buffer = std::stringstream();
				buffer << stream.rdbuf();
				std::string content = buffer.str();

				ResponseFilesSet rv;
				rv.file_content = std::move(content);
				return processAsResponseFile(rv, p, options);
#else
				struct FileReader {
					int handle = -1;

					char *data = nullptr;
					size_t size = 0;

					std::string filespec;

					~FileReader() {
						if (handle >= 0) {
							::close(handle);
							handle = -1;
						}
						free(data);
					}

					std::optional<ErrorResponse> open(const path &filepath) {
						filespec = reinterpret_cast<const char *>(filepath.generic_u8string().c_str());
						handle = ::open(filespec.c_str(), O_RDONLY);
						if (handle < 0) {
							auto e = errno;
							return ErrorResponse{std::errc::io_error, std::format("cannot open file \"{}\": error {}:{}", filespec, e, strerror(e))};
						}
						return std::nullopt;
					}

					bool reserve_bufferspace(size_t amount) {
						amount += 4;  // plenty space for sentinels
						data = reinterpret_cast<char *>(malloc(amount));
						if (data == nullptr) {
							//	throw std::bad_alloc();
							return false;
						}
						size = amount;
						return true;
					}

					std::expected<size_t, ErrorResponse> readAllContent(size_t amount) {
						assert(size > amount);

						if (!reserve_bufferspace(amount)) {
							return std::unexpected{ErrorResponse{std::errc::not_enough_memory, std::format("out of memory while processing file \"{}\".", filespec)}};
						}
						assert(data != nullptr);
						[[assume(data != nullptr)]]
						__assume(data != nullptr);

						auto rv = ::read(handle, data, amount);
						if (rv < 0) {
							auto e = errno;
							return std::unexpected{ErrorResponse{std::errc::io_error, std::format("cannot read file content of file \"{}\": error {}:{}", filespec, e, strerror(e))}};
						}
						// write string sentinel:
						data[rv] = 0;
						return rv;
					}
				};

				FileReader reader;
				auto o = reader.open(p);
				if (o)
					return std::unexpected{o.value()};
				auto r = reader.readAllContent(filesize);
				if (!r.has_value())
					return std::unexpected{r.error()};
				std::string_view v(reader.data, reader.size);

				ResponseFilesSet rv;
				rv.file_content = v;
				return processAsResponseFile(rv, p, options);
#endif
			}

			return std::unexpected{ErrorResponse{std::errc::no_such_file_or_directory, std::format("file \"{}\" does not exist.", p.generic_string())}};
		});

	}

	ResponseFileParseResult processAsResponseFile(const std::string &response_file_content, const path& filepath, const ResponsefileProcessingOptions &options) {
		ResponseFilesSet rv;
		rv.file_content = response_file_content;
		return processAsResponseFile(rv, filepath, options);
	}

	ResponseFileParseResult processAsResponseFile(std::string &&response_file_content, const path& filepath, const ResponsefileProcessingOptions &options) {
		ResponseFilesSet rv;
		rv.file_content = response_file_content;
		return processAsResponseFile(rv, filepath, options);
	}

	static inline bool is_blank(const char c) {
		return c == ' ' || c == '\t';
	}

	static inline bool is_eol(const char c) {
		return c == '\r' || c == '\n' || c == '\0';
	}

	ResponseFileParseResult processAsResponseFile(ResponseFileParseResult &&data, const path& filepath, const ResponsefileProcessingOptions &options) {
		if (!data.has_value()) {
			return std::move(data);
		}

		auto& dv = data.value();
		std::string_view d(dv.file_content);
		ResponseFilesSet::list lines;
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
					while (!is_eol(ptr[i])) {
						i++;
					}
					continue;
				}
				//[[fallthrough]]
			default:
				auto start = i;
				// path MAY have INTERNAL whitespace:
				while (!is_eol(ptr[i])) {
					i++;
				}
				i--;
				// but trim off trailing whitespace!
				while (is_blank(ptr[i])) {
					i--;
				}
				i++;

				std::string_view line(ptr + start, i - start);
				path f(line);
				auto lr = locateFile(f, filepath, options.search_paths, options.specfile_path_is_also_search_path, options.accept_absolute_paths, options.accept_relative_paths);
				if (!lr.has_value()) {
					failure_count++;
					if (!options.tolerated_nonexist_ratio) {
						return std::unexpected{ErrorResponse{std::errc::wrong_protocol_type, std::format("file \"{}\" is not a response file (a given path \"{}\" does not exist).", filepath.generic_string(), line)}};
					}

					// can we already estimate the 'goodness' of the aspirant response file (before we go and parse the entire bloody thing!)?
					float score = failure_count;
					score /= lines.capacity();   // baseline: higher or equal to the amount of lines parsed, so the score is OPTIMISTIC!
					if (options.tolerated_nonexist_ratio < score) {
						return std::unexpected{ErrorResponse{std::errc::wrong_protocol_type, std::format("file \"{}\" is not a response file: the (estimated) line faults score is too high: {}/{} > {}.", filepath.generic_string(), failure_count, lines.capacity(), options.tolerated_nonexist_ratio)}};
					}
				}
				
				lines.push_back(line);
				continue;
			}
		}

		dv.files = std::move(lines);

		return std::move(data);
	}

}

