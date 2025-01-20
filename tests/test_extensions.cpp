#include "stdafx.h"

#include "extensions/extensions.h"

#include <gtest/gtest.h>

#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT_64
#else
#define ENVIRONMENT_32
#endif
#endif

using namespace janecekvit;
using namespace std::string_literals;

namespace framework_tests
{
class test_extensions : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}
};

TEST_F(test_extensions, TestExecuteOnContainerObject)
{
	std::unordered_map<size_t, std::string> myMap = {
		{ 5, "tezko" }
	};
	auto value1 = extensions::execute_on_container(myMap, 5, [](std::string& oResult)
		{
			return 20;
		});

	auto value2 = extensions::execute_on_container(myMap, 6, [](std::string& oResult)
		{
			return 10;
		});

	ASSERT_EQ(value1, 20);
	ASSERT_EQ(value2, 0);
}

TEST_F(test_extensions, TestExecuteOnContainerObjectConst)
{
	const std::unordered_map<size_t, std::string> myMap = {
		{ 5, "tezko" }
	};
	auto value1 = extensions::execute_on_container(myMap, 5, [](const std::string& oResult)
		{
			return 20;
		});

	auto value2 = extensions::execute_on_container(myMap, 6, [](const std::string& oResult)
		{
			return 10;
		});

	ASSERT_EQ(value1, 20);
	ASSERT_EQ(value2, 0);
}

TEST_F(test_extensions, TestRecast)
{
	struct base
	{
		virtual ~base() = default;
	};

	struct derived : public base
	{
	};

	auto ptr = std::make_unique<derived>();
	ASSERT_TRUE(static_cast<bool>(ptr));

	auto ptr2 = extensions::recast<derived, base>(std::move(ptr));
#pragma warning(suppress : 26800)
	ASSERT_FALSE(static_cast<bool>(ptr));
	ASSERT_TRUE(static_cast<bool>(ptr2));

	auto ptr3 = extensions::recast<base, derived>(std::move(ptr2));
	ASSERT_FALSE(static_cast<bool>(ptr));
#pragma warning(suppress : 26800)
	ASSERT_FALSE(static_cast<bool>(ptr2));
	ASSERT_TRUE(static_cast<bool>(ptr3));
}

TEST_F(test_extensions, TestTupleGenerate)
{
	auto callback = [](auto&&... oArgs) -> int
	{
		auto tt = std::forward_as_tuple(oArgs...);
		return std::get<0>(tt);
	};
	auto oResultGenerator = extensions::tuple::generate<10>(callback);
	ASSERT_EQ(oResultGenerator, std::make_tuple(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));
}

TEST_F(test_extensions, TestTuplePrint)
{
	auto oStream = extensions::tuple::print(std::make_tuple(1, 2, 3, "1", "10"), std::string(", "));
	ASSERT_EQ(oStream.str(), "1, 2, 3, 1, 10, "s);
}

TEST_F(test_extensions, TestNumeric)
{
	auto result = extensions::numeric::factorial<5>();
	ASSERT_EQ(result.value, (size_t) 120);
}

TEST_F(test_extensions, TestHashFunction)
{
	std::string s("ano");
	int i = 5;
	size_t uHash = extensions::hash::combine(s, i);

#if defined ENVIRONMENT_64
	ASSERT_EQ(uHash, static_cast<size_t>(8002369318281051212));
#elif defined ENVIRONMENT_32
	ASSERT_EQ(uHash, static_cast<size_t>(730160148));
#endif
}

} // namespace framework_tests
