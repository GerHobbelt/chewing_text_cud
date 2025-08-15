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

#pragma once

#include "Base.hpp"
#include "ReadFileContents.hpp"
#include "ContentSplitting.hpp"


namespace text_processing {

	using std::filesystem::path;

	struct ResponsefileProcessingOptions {
		float tolerated_nonexist_ratio = 0.0f;

		bool accept_absolute_paths{true};
		bool accept_relative_paths{true};
		bool accept_comment_lines{true};

		bool specfile_path_is_also_search_path{true};
	};

	struct ResponseFilesSet : public FileContent {
		// a list of target file paths, extracted from the response file.
		searchPaths files;

		ResponseFilesSet() = default;
		ResponseFilesSet(const TextBuffer &s);
		ResponseFilesSet(TextBuffer &&s);
	};

	using ResponseFileParseResult = std::expected<ResponseFilesSet, ErrorResponse>;

	ResponseFileParseResult processAsResponseFile(const path& filepath, const searchPaths& search_paths = {}, const ResponsefileProcessingOptions &options = {});
	ResponseFileParseResult processAsResponseFile(const std::string &response_file_content, const std::string& filepath, const searchPaths& search_paths = {}, const ResponsefileProcessingOptions &options = {});
	ResponseFileParseResult processAsResponseFile(std::string &&response_file_content, const std::string& filepath, const searchPaths& search_paths = {}, const ResponsefileProcessingOptions &options = {});

}

