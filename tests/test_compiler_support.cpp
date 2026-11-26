#include "compatibility/compiler_support.h"

#include <gtest/gtest.h>

namespace framework_tests
{

class test_compiler_support : public ::testing::Test
{
};

TEST_F(test_compiler_support, TestPrefixedFeatureMacrosMatchLegacyNames)
{
#if defined(JANECEKVIT_HAS_JTHREAD)
	constexpr bool prefixed_jthread = true;
#else
	constexpr bool prefixed_jthread = false;
#endif
#if defined(HAS_JTHREAD)
	constexpr bool legacy_jthread = true;
#else
	constexpr bool legacy_jthread = false;
#endif
#if defined(JANECEKVIT_HAS_STD_FORMAT)
	constexpr bool prefixed_format = true;
#else
	constexpr bool prefixed_format = false;
#endif
#if defined(HAS_STD_FORMAT)
	constexpr bool legacy_format = true;
#else
	constexpr bool legacy_format = false;
#endif

	ASSERT_EQ(prefixed_jthread, legacy_jthread);
	ASSERT_EQ(prefixed_format, legacy_format);
}

} // namespace framework_tests
