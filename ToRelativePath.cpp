
#include "Base.hpp"

namespace text_processing {

	namespace fs = std::filesystem;

	// Return a relative path, that's relative to the given `base_filepath`.
	std::expected<path, ErrorResponse> ConvertToRelativePath(const path &filepath, const path &base_filepath) {
		std::error_code ec;
		path d = std::filesystem::proximate(filepath, base_filepath, ec);
		if (ec) {
			return std::unexpected{ErrorResponse{std::errc::invalid_argument, std::format("cannot construct a relative path from \"{}\" with base/root \"{}\": error {}:{}", filepath.generic_string(), base_filepath.generic_string(), ec.value(), ec.message())}};
		}
		if (d.empty() || d.is_absolute()) {
			return std::unexpected{ErrorResponse{std::errc::invalid_argument, std::format("cannot construct a relative path from \"{}\" with base/root \"{}\".", filepath.generic_string(), base_filepath.generic_string())}};
		}
		return d;
	}

}

