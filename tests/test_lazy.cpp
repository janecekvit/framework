#include "extensions/lazy.h"

#include <gtest/gtest.h>

using namespace janecekvit;

namespace framework_tests
{
class test_lazy : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}
};

TEST_F(test_lazy, TestLazyAction)
{
	// value by capture
	int result = 0;
	auto lazy_action = extensions::lazy_action<void>([&]()
		{
			result = 5;
		});

	ASSERT_EQ(result, 0);
	lazy_action();
	ASSERT_EQ(result, 5);
}

TEST_F(test_lazy, TestLazyActionReturnValue)
{
	// return value
	int result = 0;
	auto lambda = [&]()
	{
		return result;
	};
	auto lazy_action = extensions::lazy_action<int>(std::move(lambda));
	ASSERT_EQ(result, 0);
	result = 5;
	ASSERT_EQ(lazy_action(), 5);
}

TEST_F(test_lazy, TestLazyActionParametersDefaultValues)
{
	// default parameter value
	auto lambda = [&](int i, int j)
	{
		return i + j;
	};
	auto lazy_action = extensions::lazy_action<int, int, int>(std::move(lambda), 5, 6);
	ASSERT_EQ(lazy_action(), 11);
}

TEST_F(test_lazy, TestLazyActionChangedValuesByParameter)
{
	// update value by parameters value
	auto lambda = [&](int i, int j)
	{
		return i + j;
	};
	auto lazy_action = extensions::lazy_action<int, int, int>(std::move(lambda), 5, 6);
	ASSERT_EQ(lazy_action(5, 9), 14); // throw out old values
}

TEST_F(test_lazy, TestLazyMethod)
{
	// value by capture
	int result = 0;
	auto lazy_action = extensions::lazy_action<void>([&]()
		{
			result = 5;
		});

	ASSERT_EQ(result, 0);
	lazy_action();
	ASSERT_EQ(result, 5);
}

TEST_F(test_lazy, TestLazyMethodReturnValue)
{
	// return value
	int result = 0;
	auto lambda = [&]()
	{
		return result;
	};
	auto lazy_action = extensions::make_lazy(std::move(lambda));
	ASSERT_EQ(result, 0);
	result = 5;
	ASSERT_EQ(lazy_action(), 5);
}

TEST_F(test_lazy, TestLazyMethodParametersDefaultValues)
{
	// default parameter value
	auto lambda = [&](int i, int j)
	{
		return i + j;
	};
	auto lazy_action = extensions::make_lazy(std::move(lambda), 5, 6);
	ASSERT_EQ(lazy_action(), 11);
}

TEST_F(test_lazy, TestLazyMethodChangedValuesByParameter)
{
	// update value by parameters value
	auto lambda = [&](int i, int j)
	{
		return i + j;
	};
	auto lazy_action = extensions::make_lazy(std::move(lambda), 5, 6);
	ASSERT_EQ(lazy_action(5, 9), 14); // throw out old values
}

TEST_F(test_lazy, TestLazyLvalueArguments)
{
	int first = 1;
	int second = 2;
	auto action = extensions::make_lazy([](int a, int b)
		{
			return a + b;
		},
		first, second);

	second = 10;
	ASSERT_EQ(11, action());
}

} // namespace framework_tests
