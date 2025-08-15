#pragma once

#include "Base.hpp"

#include <string>
#include <format>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
#include <cmath>
#include <cstdint>

namespace text_processing {

	// ripped from https://en.cppreference.com/w/cpp/filesystem/file_size.html
	struct HumanReadable
	{
		std::uintmax_t size{};

		std::string to_string() const
		{
			int o{0};
			double mantissa = size;
			for (; mantissa >= 1024.; mantissa /= 1024., ++o);
			std::string rv = std::format("{:.1f}{}", mantissa, "BKMGTPE"[o]);
			if (o) {
				rv += std::format("B ({})", size);
			}
			return rv;
		}

	private:
		friend std::ostream& operator<<(std::ostream& os, HumanReadable hr)
		{
			int o{0};
			double mantissa = hr.size;
			for (; mantissa >= 1024.; mantissa /= 1024., ++o);
			os << std::ceil(mantissa * 10.) / 10. << "BKMGTPE"[o];
			return o ? os << "B (" << hr.size << ')' : os;
		}
	};

	// ------------------------------------------------------------------

}

