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

#include "ResponseFileHandling.hpp"

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

	ResponseFilesSet::ResponseFilesSet(const TextBuffer &s) :
		FileContent(s) {
	}
	ResponseFilesSet::ResponseFilesSet(TextBuffer &&s) :
		FileContent(std::move(s)) {
	}

	// -----------------------------------------------------------------------------------------------------

	using line_list = decltype(ExtendedFileContent::lines);

	//private:
	static ResponseFileParseResult internalProcessAsResponseFile(ResponseFilesSet &data, const line_list& lines, const std::string& filepath, const searchPaths& search_paths, const ResponsefileProcessingOptions &options);

	ResponseFileParseResult processAsResponseFile(const path &filepath, const searchPaths& search_paths, const ResponsefileProcessingOptions &options) {
		FileContentProcessingOptions proc_opts{
		.mode = FileContentProcessingOptions::ParseMode::ToTextLines,
		.trim_outer_whitespace = true,
		.accept_comment_lines = true
		};
		return processFileEx(filepath, search_paths, proc_opts).and_then([&](ExtendedFileContent &&content) -> ResponseFileParseResult {
			ResponseFilesSet rv(content.file_content);
			return internalProcessAsResponseFile(rv, content.lines, filepath.generic_string(), search_paths, options);
		});
	}

	ResponseFileParseResult processAsResponseFile(const std::string &response_file_content, const std::string& filepath, const searchPaths& search_paths, const ResponsefileProcessingOptions &options) {
		FileContentProcessingOptions proc_opts{
		.mode = FileContentProcessingOptions::ParseMode::ToTextLines,
		.trim_outer_whitespace = true,
		.accept_comment_lines = true
		};
		TextBuffer buf(response_file_content);
		ExtendedFileContent exfcontent;
		exfcontent.file_content = std::move(buf);
		std::error_code ec;
		if (exfcontent.parseContentAsLines(proc_opts, ec), ec) {
			return std::unexpected{ErrorResponse{std::errc::not_enough_memory, std::format("failure while parsing buffer space ({}) for response file \"{}\": error {}:{}", HumanReadable(exfcontent.file_content.content_size()).to_string(), filepath, ec.value(), ec.message())}};
		}
		ResponseFilesSet rv(std::move(exfcontent.file_content));
		return internalProcessAsResponseFile(rv, exfcontent.lines, filepath, search_paths, options);
	}

	ResponseFileParseResult processAsResponseFile(std::string &&response_file_content, const std::string& filepath, const searchPaths& search_paths, const ResponsefileProcessingOptions &options) {
		FileContentProcessingOptions proc_opts{
		.mode = FileContentProcessingOptions::ParseMode::ToTextLines,
		.trim_outer_whitespace = true,
		.accept_comment_lines = true
		};
		TextBuffer buf(response_file_content);
		ExtendedFileContent exfcontent;
		exfcontent.file_content = std::move(buf);
		std::error_code ec;
		if (exfcontent.parseContentAsLines(proc_opts, ec), ec) {
			return std::unexpected{ErrorResponse{std::errc::not_enough_memory, std::format("failure while parsing buffer space ({}) for response file \"{}\": error {}:{}", HumanReadable(exfcontent.file_content.content_size()).to_string(), filepath, ec.value(), ec.message())}};
		}
		ResponseFilesSet rv(std::move(exfcontent.file_content));
		return internalProcessAsResponseFile(rv, exfcontent.lines, filepath, search_paths, options);
	}

	static ResponseFileParseResult internalProcessAsResponseFile(ResponseFilesSet &data, const line_list& lines, const std::string& filepath, const searchPaths& search_paths, const ResponsefileProcessingOptions &options) {
		// reserve the number of lines that were be found: each will result in a target path if all goes well...
		data.files.reserve(lines.size());

		uintmax_t failure_count = 0;
		for (const auto line : lines) {
			path f(line);
			auto lr = locateFile(f, filepath, search_paths, options.specfile_path_is_also_search_path, options.accept_absolute_paths, options.accept_relative_paths);
			if (!lr.has_value()) {
				failure_count++;
				if (!options.tolerated_nonexist_ratio) {
					return std::unexpected{ErrorResponse{std::errc::wrong_protocol_type, std::format("file \"{}\" is not a response file (a given path \"{}\" does not exist).", filepath, line)}};
				}

				// can we already estimate the 'goodness' of the aspirant response file (before we go and parse the entire bloody thing!)?
				float score = failure_count;
				score /= data.files.capacity();   // baseline: higher or equal to the amount of lines parsed, so the score is OPTIMISTIC!
				if (options.tolerated_nonexist_ratio < score) {
					return std::unexpected{ErrorResponse{std::errc::wrong_protocol_type, std::format("file \"{}\" is not a response file: the (estimated) line faults score is too high: {}/{} > {}.", filepath, failure_count, data.files.capacity(), options.tolerated_nonexist_ratio)}};
				}
			}

			//lines.push_back(line);
			data.files.push_back(std::move(lr.value()));
		}

		return std::move(data);
	}

}

