#include "stdafx.h"

#include "thread/async.h"

#include <future>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

using namespace janecekvit::thread;

constexpr const size_t thread_size = 4;

namespace framework_tests
{

class test_async : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}
};

TEST_F(test_async, Create)
{
	std::atomic<int> counter = 0;
	auto result = async::create([&counter]()
		{
			counter++;
		});

	result.get();
	ASSERT_EQ(counter, 1);
}

TEST_F(test_async, CreateWithParameter)
{
	std::atomic<int> counter = 0;
	auto result = async::create([&counter](int i)
		{
			counter = i;
		},
		5);

	result.get();
	ASSERT_EQ(counter, 5);
}

TEST_F(test_async, CreateWithResult)
{
	auto result = async::create([]()
		{
			return 5;
		});
	ASSERT_EQ(result.get(), 5);
}

TEST_F(test_async, CreateWithResultAndParam)
{
	auto result = async::create([](int i)
		{
			return i;
		},
		10);
	ASSERT_EQ(result.get(), 10);
}

TEST_F(test_async, CreateWithResultAndTwoParams)
{
	auto result = async::create([](int i, int j)
		{
			return i + j;
		},
		10, 15);
	ASSERT_EQ(result.get(), 25);
}

TEST_F(test_async, CreateWithException)
{
	auto task([]()
		{
			throw std::exception();
		});

	auto result = async::create(std::move(task));
	ASSERT_THROW(result.get(), std::exception);
}

} // namespace framework_tests
