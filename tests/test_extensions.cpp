#include "extensions/extensions.h"

#include <gtest/gtest.h>

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
	auto value1 = extensions::execute_on_container(myMap, 5, [](std::string& )
		{
			return 20;
		});

	auto value2 = extensions::execute_on_container(myMap, 6, [](std::string& )
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
	auto value1 = extensions::execute_on_container(myMap, 5, [](const std::string&)
		{
			return 20;
		});

	auto value2 = extensions::execute_on_container(myMap, 6, [](const std::string&)
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
	ASSERT_FALSE(static_cast<bool>(ptr));
	ASSERT_TRUE(static_cast<bool>(ptr2));

	auto ptr3 = extensions::recast<base, derived>(std::move(ptr2));
	ASSERT_FALSE(static_cast<bool>(ptr));
	ASSERT_FALSE(static_cast<bool>(ptr2));
	ASSERT_TRUE(static_cast<bool>(ptr3));
}

TEST_F(test_extensions, TestTupleGenerate)
{
	auto callback = [](auto&&... oArgs) -> int
	{
		auto tt = std::forward_as_tuple(oArgs...);
		return static_cast<int>(std::get<0>(tt));
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
	std::string s1("ano");
	std::string s2("ano");
	std::string s3("different");
	int i1 = 5;
	int i2 = 5;
	int i3 = 10;
	
	size_t hash1 = extensions::hash::combine(s1, i1);
	size_t hash2 = extensions::hash::combine(s2, i2);
	size_t hash3 = extensions::hash::combine(s3, i3);
	size_t hash4 = extensions::hash::combine(s1, i3);

	// Same inputs should produce same hash
	ASSERT_EQ(hash1, hash2);
	
	// Different inputs should produce different hashes (with high probability)
	ASSERT_NE(hash1, hash3);
	ASSERT_NE(hash1, hash4);
	ASSERT_NE(hash3, hash4);
	
	// Hash should not be zero (basic sanity check)
	ASSERT_NE(hash1, 0);
	ASSERT_NE(hash3, 0);
}

} // namespace framework_tests
