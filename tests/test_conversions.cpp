#include "synchronization/concurrent.h"
#include "utility/conversions.h"

#include <gtest/gtest.h>
#include <list>

using namespace janecekvit;

namespace framework_tests
{
class test_conversions : public ::testing::Test
{
};

TEST_F(test_conversions, TestContainerToString)
{
	ASSERT_EQ("a, b, c", conversions::to_string<std::string>(std::vector<std::string>{ "a", "b", "c" }));
	ASSERT_EQ("a", conversions::to_string<std::string>(std::list<std::string>{ "a" }));
	ASSERT_EQ("", conversions::to_string<std::string>(std::vector<std::string>{}));
}

TEST_F(test_conversions, TestUnconvertibleWideStringReturnsEmpty)
{
	ASSERT_EQ("", conversions::to_string(std::wstring(L"\u4e2d")));
}

TEST_F(test_conversions, TestWideContainerToString)
{
	ASSERT_EQ(L"a, b", conversions::to_string<std::wstring>(std::vector<std::wstring>{ L"a", L"b" }));
}

TEST_F(test_conversions, TestConcurrentContainerToString)
{
	auto container = synchronization::concurrent::vector<std::string>();
	container.exclusive()->emplace_back("a");
	container.exclusive()->emplace_back("b");

	ASSERT_EQ("a, b", conversions::to_string<std::string>(container));
}

} // namespace framework_tests
