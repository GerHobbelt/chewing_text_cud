#pragma once

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

namespace text_processing {

	using std::filesystem::path;

	struct ErrorResponse {
		std::errc code;
		std::string message;
	};

	typedef std::vector<path> searchPaths;

	std::expected<path, ErrorResponse> locateFile(const path &filepath, const path &source_filepath = std::filesystem::current_path(), const searchPaths& search_paths = {}, bool specfile_path_is_also_search_path = true, bool accept_absolute_paths = true, bool accept_relative_paths = true);

	// Return a relative path, that's relative to the given `base_filepath`.
	std::expected<path, ErrorResponse> ConvertToRelativePath(const path &filepath, const path &base_filepath = std::filesystem::current_path());

	// Return a canonical=normalized absolute path for `filepath`. When the `filepath` is relative, it is based relative to the `base_filepath`, which, when not an absolute path itself, is based relative to the `current_working_directory` path (which, when empty, assumes `fs::current_path()`).
	//
	// Simplified: retval = current_working_directory / base_filepath / filepath
	path ConvertToAbsoluteNormalizedPath(const path &filepath, const path &base_filepath, const path &current_working_directory = std::filesystem::current_path());

	path NormalizePathToUnixSeparators(const path &filepath);

	// ---------------------------------------------------------------

	class TextBuffer {
	protected:
		char *_data = nullptr;
		size_t _size = 0;
		size_t _capacity = 0;

	public:
		static constexpr const size_t sentinel_size = 4;

		//TextBuffer() = default;
		TextBuffer(const char *str);
		TextBuffer(const char *str, size_t length, size_t requested_buffer_size = 0);
		TextBuffer(const std::string_view &str, size_t requested_buffer_size = 0);
		TextBuffer(size_t requested_buffer_size = 0);
	protected:
		// helper for the other constructors: code deduplication
		explicit TextBuffer(size_t requested_buffer_size, const std::string_view &str);
	public:

		~TextBuffer();

		TextBuffer(const TextBuffer &src);
		TextBuffer(TextBuffer &&lvsrc);

		TextBuffer& operator=(const TextBuffer& other);
		TextBuffer& operator=(TextBuffer&& other);

		TextBuffer& operator=(const std::string_view &str);

		void reserve(size_t amount);
		void reserve(size_t amount, std::error_code &ec);

		char *data() const {
			return _data;
		}
		size_t content_size() const {
			return _size;
		}
		size_t capacity() const {
			return _capacity;
		}
		std::string_view content_view() const {
			return {_data, _size};
		}
		std::string_view capacity_view() const {
			return {_data, _capacity};
		}

		void set_content_size(size_t amount);

		void CopyAndPrepare(const char *str, size_t strlength, size_t requested_buffer_size, std::error_code &ec);

		// nuke/reset the Textbuffer
		void clear(void);
	};



}

