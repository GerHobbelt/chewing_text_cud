
#include "Base.hpp"

namespace text_processing {

	namespace fs = std::filesystem;

	std::expected<path, ErrorResponse> locateFile(const path &filepath, const path &source_filepath, const searchPaths& search_paths, bool specfile_path_is_also_search_path, bool accept_absolute_paths, bool accept_relative_paths) {
		if (filepath.is_relative()) {
			path cwd = fs::current_path();

			if (!accept_relative_paths) {
				return std::unexpected{ErrorResponse{std::errc::invalid_argument, std::format("relative paths, such as file/path \"{}\", are not accepted.", filepath.generic_string())}};
			}

			// apply search path:
			if (!specfile_path_is_also_search_path || source_filepath.empty()) {
				path f = ConvertToAbsoluteNormalizedPath(filepath, cwd, cwd);
				if (std::filesystem::exists(f)) {
					return f;
				}
			}
			else if (specfile_path_is_also_search_path && filepath != source_filepath) {
				path f = ConvertToAbsoluteNormalizedPath(filepath, source_filepath.parent_path(), cwd);
				if (std::filesystem::exists(f)) {
					return f;
				}
			}

			for (int i = 0, l = search_paths.size(); i < l; i++) {
				path f = search_paths[i];
				f = ConvertToAbsoluteNormalizedPath(filepath, f, cwd);
				if (std::filesystem::exists(f)) {
					return f;
				}
			}

			//ErrorResponse e{std::errc::no_such_file_or_directory, std::format("file \"{}\" does not exist.", filepath.generic_string())};
			//return std::unexpected{e};
			return std::unexpected{ErrorResponse{std::errc::no_such_file_or_directory, std::format("file/path \"{}\" does not exist.", filepath.generic_string())}};
		}
		// else: input is an absolute path, so no search_paths traversal or anything!
		if (!accept_absolute_paths) {
			return std::unexpected{ErrorResponse{std::errc::invalid_argument, std::format("absolute paths, such as file/path \"{}\", are not accepted.", filepath.generic_string())}};
		}

		path f = ConvertToAbsoluteNormalizedPath(filepath, {}, {});
		if (std::filesystem::exists(f)) {
			return f;
		}

		//ErrorResponse e{std::errc::no_such_file_or_directory, std::format("file \"{}\" does not exist.", filepath.generic_string())};
		//return std::unexpected{e};
		return std::unexpected{ErrorResponse{std::errc::no_such_file_or_directory, std::format("file/path \"{}\" does not exist.", filepath.generic_string())}};
	}

}

