
#include "Base.hpp"

namespace text_processing {

	namespace fs = std::filesystem;

	path NormalizePathToUnixSeparators(const path &filepath) {
		path f = std::filesystem::weakly_canonical(filepath);
		//f.preferred_separator = TEXT('/');
		f = f.lexically_normal();
#if defined(_WIN32)
		auto s = f.generic_string();
		return s;
#else
		return f;
#endif
	}

	// Return a canonical=normalized absolute path for `filepath`. When the `filepath` is relative, it is based relative to the `base_filepath`, which, when not an absolute path itself, is based relative to the `current_working_directory` path (which, when empty, assumes `fs::current_path()`).
	//
	// Simplified: retval = current_working_directory / base_filepath / filepath
	path ConvertToAbsoluteNormalizedPath(const path &filepath, const path &base_filepath, const path &current_working_directory) {
		if (filepath.is_relative()) {
			path f = filepath;

			// apply base_filepath, if that one is non-empty.
			if (!base_filepath.empty()) {
				f = base_filepath / filepath;

				if (f.is_absolute()) {
					return NormalizePathToUnixSeparators(f);
				}
			}

			path cwd = current_working_directory;
			if (cwd.empty()) {
				cwd = fs::current_path();
			} else if (cwd.is_relative()) {
				if (cwd != base_filepath) {
					// edge case where CWD is specified as a relative path itself: prepend the system/application's absolute path PWD then:
					cwd = fs::current_path() / cwd;
				}
				else {
					// cwd == base_filepath, both relative --> do not append base_filepath *twice* to `f`:
					cwd = fs::current_path();
				}
			}
			f = cwd / f;
			return NormalizePathToUnixSeparators(f);
		} else {
			// `filepath` was already absolute itself: pass it on as-is.
			return NormalizePathToUnixSeparators(filepath);
		}
	}

}

