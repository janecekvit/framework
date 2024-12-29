#include "stdafx.h"

#include "extensions/finally.h"

#include <gtest/gtest.h>

using namespace janecekvit;
namespace FrameworkTesting
{
class test_finally : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}
};

#ifdef __cpp_lib_concepts
TEST_F(test_finally, TestFinallyAction)
{
	int result = 0;
	{
		auto finally = extensions::final_action([&]()
			{
				result = 5;
			});

		ASSERT_EQ(result, 0);
	}

	ASSERT_EQ(result, 5);
}
TEST_F(test_finally, TestFinallyActionWithException)
{
	int result = 0;
	bool bExceptionThrow = false;
	{
		auto finally = extensions::final_action([&]()
			{
				result = 5;
				throw std::exception("HH");
				result = 10;
			},
			[&](const std::exception& ex)
			{
				ASSERT_THROW(
					{
						bExceptionThrow = true;
						throw ex;
					},
					std::exception);
			});

		ASSERT_EQ(result, 0);
	}

	ASSERT_EQ(result, 5); // value still on 5
	ASSERT_TRUE(bExceptionThrow);
}

TEST_F(test_finally, TestFinallyMethod)
{
	int result = 0;
	{
		auto finally = extensions::finally([&]()
			{
				result = 5;
			});

		ASSERT_EQ(result, 0);
	}

	ASSERT_EQ(result, 5);
}
TEST_F(test_finally, TestFinallyMethodWithException)
{
	int result = 0;
	bool bExceptionThrow = false;
	{
		auto finally = extensions::finally([&]()
			{
				result = 5;
				throw std::exception("HH");
				result = 10;
			},
			[&](const std::exception& ex)
			{
				ASSERT_THROW(
					{
						bExceptionThrow = true;
						throw ex;
					},
					std::exception);
			});

		ASSERT_EQ(result, 0);
	}

	ASSERT_EQ(result, 5); // value still on 5
	ASSERT_TRUE(bExceptionThrow);
}
#endif
} // namespace FrameworkTesting