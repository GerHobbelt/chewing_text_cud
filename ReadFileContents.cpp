
#include "ReadFileContents.hpp"

#include "PrivateUtilities.hpp"

#if defined(_WIN32)
#ifndef _CRT_DECLARE_NONSTDC_NAMES
#define _CRT_DECLARE_NONSTDC_NAMES  1
#endif
#include <io.h>
#include <fcntl.h>
#include <string.h>
#endif


namespace text_processing {

	namespace fs = std::filesystem;

	FileReader::~FileReader() {
		if (handle >= 0) {
			::close(handle);
			handle = -1;
		}
		data.clear();
	}

	std::optional<ErrorResponse> FileReader::open(const path &filepath) {
		filespec = reinterpret_cast<const char *>(filepath.generic_u8string().c_str());
		handle = ::open(filespec.c_str(), O_RDONLY);
		if (handle < 0) {
			auto e = errno;
			return ErrorResponse{std::errc::io_error, std::format("cannot open file \"{}\": error {}:{}", filespec, e, strerror(e))};
		}
		return std::nullopt;
	}

	bool FileReader::reserve_bufferspace(size_t amount) {
		amount += TextBuffer::sentinel_size;  // plenty space for sentinels
		if (data.capacity() < amount) {
			std::error_code ec;
			data.reserve(amount, ec);
			return !ec;
		}
		return true;
	}

	std::expected<size_t, ErrorResponse> FileReader::readAllContent(size_t amount) {
		if (!reserve_bufferspace(amount + TextBuffer::sentinel_size)) {
			return std::unexpected{ErrorResponse{std::errc::not_enough_memory, std::format("out of memory while processing file \"{}\".", filespec)}};
		}
		assert(data.capacity() > amount);
		assert(data.data() != nullptr);
		[[assume(data.data() != nullptr)]]
		__assume(data.data() != nullptr);

		auto rv = ::read(handle, data.data(), amount);
		if (rv < 0) {
			auto e = errno;
			return std::unexpected{ErrorResponse{std::errc::io_error, std::format("cannot read file content of file \"{}\": error {}:{}", filespec, e, strerror(e))}};
		}
		// write string sentinel:
		data.data()[rv] = 0;

		// mark the buffer space used (excluding the sentinel) as occupied/content.
		data.set_content_size(rv);

		return rv;
	}

	// ------------------------------------------------------------------------------------

	FileContent::FileContent(const TextBuffer &s) :
		file_content(s) {
	}

	FileContent::FileContent(TextBuffer &&s) :
		file_content(std::move(s)) {
	}


	ExtendedFileContent::ExtendedFileContent(const TextBuffer &s) :
		FileContent(s) {
	}

	ExtendedFileContent::ExtendedFileContent(TextBuffer &&s) :
		FileContent(std::move(s)) {
	}


	FileContentParseResult processFile(const path& filepath, const searchPaths& search_paths) {
		return locateFile(filepath, filepath, search_paths).and_then([](path &&p) -> FileContentParseResult {
			// https://medium.com/@nerudaj/tuesday-coding-tip-78-many-ways-of-reading-a-file-in-c-e66191dc60e3

			std::error_code ec;
			if (const std::uintmax_t filesize = fs::file_size(p, ec); ec) {
				if (false) std::cout << p.generic_string() << " : " << ec.message() << '\n';
				return std::unexpected{ErrorResponse{std::errc::io_error, std::format("file size for file \"{}\" cannot be determined; {}", p.generic_string(), ec.message())}};
			} else {
				if (false) std::cout << p.generic_string() << " size = " << HumanReadable{filesize} << '\n';

				FileReader reader;
				auto o = reader.open(p);
				if (o)
					return std::unexpected{o.value()};
				auto r = reader.readAllContent(filesize);
				if (!r.has_value())
					return std::unexpected{r.error()};

				FileContent rv(reader.data);
				return rv;
			}

			return std::unexpected{ErrorResponse{std::errc::no_such_file_or_directory, std::format("file \"{}\" does not exist.", p.generic_string())}};
		});
	}

	size_t estimateRequiredLumpSumBufferSpace(std::uintmax_t filesize, const FileContentProcessingOptions& options) {
		size_t base_amount = filesize + TextBuffer::sentinel_size;
		size_t amount = base_amount;

		using mode = FileContentProcessingOptions::ParseMode;

		FileContentProcessingOptions::ParseMode parse_mode = options.mode;
		if (parse_mode & mode::ToTextLines) {
			// no extra space needed for the text lines: those only get one std::string_view slot in lines[] each.
		}
		if (parse_mode & mode::ToParagraphs) {
			// this *very probably* rewrites the entire content, getting rid of newlines within the paragraph, etc. --> twice the costs.
			// paragraph endings (NUL) replace the newlines in the source text, so we don't need to compensate for those.
			// Ditto for the parse options: we will be rewriting the paragraph texts anyway, so a single duplicate it is --> only twice the costs.
			amount += base_amount;
		}
		if (parse_mode & mode::ToWords) {
			// 'words' will create a view for each word in the original text. Processing options will *potentially* result in
			// rewritten/cleaned-up/processed copies of these words.
			// Asian languages and several others don't do word separating whitespace, so we probably will be adding that as well.
			//
			// Apply a pessimistic heuristic/estimate for this one.
			amount += base_amount * (1.0 + 1.0/3.0);
		}
		if (parse_mode & mode::ToNGrams) {
			// these don't take up buffer space; they merely load the ngrams_list array with a zillion entries.
		}
		return amount;
	}


	ExtendedFileContentParseResult processFileEx(const path& filepath, const searchPaths& search_paths, const FileContentProcessingOptions& options) {
		return locateFile(filepath, filepath, search_paths).and_then([options](path &&p) -> ExtendedFileContentParseResult {
			// https://medium.com/@nerudaj/tuesday-coding-tip-78-many-ways-of-reading-a-file-in-c-e66191dc60e3

			std::error_code ec;
			if (const std::uintmax_t filesize = fs::file_size(p, ec); ec) {
				if (false) std::cout << p.generic_string() << " : " << ec.message() << '\n';
				return std::unexpected{ErrorResponse{std::errc::io_error, std::format("file size for file \"{}\" cannot be determined; {}", p.generic_string(), ec.message())}};
			}
			else {
				if (false) std::cout << p.generic_string() << " size = " << HumanReadable{filesize} << '\n';

				FileReader reader;
				auto o = reader.open(p);
				if (o)
					return std::unexpected{o.value()};

				size_t size_request = estimateRequiredLumpSumBufferSpace(filesize, options);
				if (reader.data.reserve(size_request, ec), ec) {
					return std::unexpected{ErrorResponse{std::errc::not_enough_memory, std::format("failure while preparing buffer space ({}) for file \"{}\": error {}:{}", HumanReadable(size_request).to_string(), p.generic_string(), ec.value(), ec.message())}};
				}

				auto r = reader.readAllContent(filesize);
				if (!r.has_value())
					return std::unexpected{r.error()};

				ExtendedFileContent rv(reader.data);

				using mode = FileContentProcessingOptions::ParseMode;

				if (options.mode & mode::ToTextLines) {
					if (rv.parseContentAsLines(options, ec), ec) {
						return std::unexpected{ErrorResponse{std::errc::no_buffer_space, std::format("failure while processing file \"{}\" into text lines: error {}:{}", p.generic_string(), ec.value(), ec.message())}};
					}
				}
				if (options.mode & mode::ToParagraphs) {
					if (rv.parseContentAsParagraphs(options, ec), ec) {
						return std::unexpected{ErrorResponse{std::errc::no_buffer_space, std::format("failure while processing file \"{}\" into text paragraphs: error {}:{}", p.generic_string(), ec.value(), ec.message())}};
					}
				}
				if (options.mode & mode::ToWords) {
					if (rv.parseContentAsWords(options, ec), ec) {
						return std::unexpected{ErrorResponse{std::errc::no_buffer_space, std::format("failure while processing file \"{}\" into words: error {}:{}", p.generic_string(), ec.value(), ec.message())}};
					}
				}
				if (options.mode & mode::ToNGrams) {
					if (rv.parseContentAsNGrams(options, ec), ec) {
						return std::unexpected{ErrorResponse{std::errc::no_buffer_space, std::format("failure while processing file \"{}\" into ngrams: error {}:{}", p.generic_string(), ec.value(), ec.message())}};
					}
				}
				return rv;
			}

			return std::unexpected{ErrorResponse{std::errc::no_such_file_or_directory, std::format("file \"{}\" does not exist.", p.generic_string())}};
		});
	}

}

