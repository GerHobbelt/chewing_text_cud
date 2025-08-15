
#include "Base.hpp"

#include <gtest/gtest.h>
#include <cstdio>

using namespace text_processing;























extern "C"
/* GTEST_API_ */ int main(int argc, const char** argv) {
	printf("Running main() from %s\n", __FILE__);
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

