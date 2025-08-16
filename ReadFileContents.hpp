#pragma once

#include "Base.hpp"

#include <cstdint>
#include <cstdio>


namespace text_processing {

	using std::filesystem::path;

	struct FileContentProcessingOptions {
		enum ParseMode : uint8_t {
			ToTextLines = 0x01,
			ToParagraphs = 0x02,
			ToWords = 0x04,
			ToNGrams = 0x08
		} mode = ToWords;

		bool trim_outer_whitespace : 1 {false};
		bool dedent_lines : 1 {false};
		bool contract_hyphenated_words_at_EOL : 1 {false};
		bool contract_lines_in_paragraph : 1 {false};				// turns each paragraph into a single line of text.
		bool stemming : 1 {false};
		bool cleanup_punctuation : 1 {false};
		bool cleanup_diacritics : 1 {false};
		bool unicode_normalization : 1 {false};

		bool accept_comment_lines : 1 {false};
	};

	struct FileContent {
		// stores the file content
		TextBuffer file_content;

		FileContent() = default;
		FileContent(const TextBuffer &s);
		FileContent(TextBuffer &&s);
	};

	struct ExtendedFileContent : public FileContent {
		// a map of pointers into the response file content, each describing one target line/word/element.
		typedef std::vector<std::string_view> list;

		typedef uint32_t ngram_id_t;

		typedef std::vector<ngram_id_t> ngram_list;

		list paragraphs{};
		list lines{};
		list words{};

		ExtendedFileContent() = default;
		ExtendedFileContent(const TextBuffer &s);
		ExtendedFileContent(TextBuffer &&s);

		void parseContentAsLines(const FileContentProcessingOptions& options, std::error_code &ec);
		void parseContentAsParagraphs(const FileContentProcessingOptions& options, std::error_code &ec);
		void parseContentAsWords(const FileContentProcessingOptions& options, std::error_code &ec);
		void parseContentAsNGrams(const FileContentProcessingOptions& options, std::error_code &ec);
	};

	using FileContentParseResult = std::expected<FileContent, ErrorResponse>;
	using ExtendedFileContentParseResult = std::expected<ExtendedFileContent, ErrorResponse>;

	FileContentParseResult processFile(const path& filepath, const searchPaths& search_paths = {});

	ExtendedFileContentParseResult processFileEx(const path& filepath, const searchPaths& search_paths = {}, const FileContentProcessingOptions& options = {});

	// -----------------------------------------------------------------------

	struct FileReader {
		FILE* handle = nullptr;

		TextBuffer data;

		std::string filespec;

		~FileReader();

		std::optional<ErrorResponse> open(const path &filepath);
		void close(void);

		bool reserve_bufferspace(size_t amount);

		std::expected<size_t, ErrorResponse> readAllContent(size_t amount);
	};


}


