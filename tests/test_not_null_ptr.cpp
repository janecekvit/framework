#include "extensions/not_null_ptr.h"

#include <gtest/gtest.h>

using namespace janecekvit;

namespace framework_tests
{
class test_not_null_ptr : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}
};

TEST_F(test_not_null_ptr, TestRawPtr)
{
	auto intPtr = new int(5);
	extensions::not_null_ptr<int*> ptr(intPtr);
	ASSERT_EQ(*ptr, 5);

	*ptr = 10;
	ASSERT_EQ(*ptr.get(), 10);

	ASSERT_TRUE(ptr == intPtr);
	ASSERT_FALSE(ptr == nullptr);

	constexpr int* intNull = nullptr;
	ASSERT_THROW(extensions::not_null_ptr<int*> x(intNull), std::exception);
}

TEST_F(test_not_null_ptr, TestRawPtrDeallocateManually)
{
	auto intPtr = new int(5);
	extensions::not_null_ptr<int*> ptr(intPtr, nullptr);
	ASSERT_EQ(*ptr, 5);

	*ptr = 10;
	ASSERT_EQ(*ptr.get(), 10);

	ASSERT_TRUE(ptr == intPtr);
	ASSERT_FALSE(ptr == nullptr);

	constexpr int* intNull = nullptr;
	ASSERT_THROW(extensions::not_null_ptr<int*> x(intNull), std::exception);

	delete intPtr;
}

TEST_F(test_not_null_ptr, TestSharedPtr)
{
	auto intPtr = std::make_shared<std::list<int>>(5);
	extensions::not_null_ptr<std::shared_ptr<std::list<int>>> ptr(intPtr);
	ASSERT_EQ((*ptr).size(), 5);

	(*ptr).push_back(10);
	ASSERT_EQ(ptr.get()->size(), 6);

	ASSERT_EQ(ptr->size(), 6);
	ASSERT_FALSE(ptr == nullptr);

	std::shared_ptr<int> intNull = nullptr;
	ASSERT_THROW(extensions::not_null_ptr<std::shared_ptr<int>> x(intNull), std::exception);
}

TEST_F(test_not_null_ptr, TestUniquePtr)
{
	auto intPtr = std::make_unique<std::list<int>>(5);
	extensions::not_null_ptr<std::unique_ptr<std::list<int>>> ptr(std::move(intPtr));
	ASSERT_EQ((*ptr).size(), 5);

	(*ptr).push_back(10);
	ASSERT_EQ(ptr.get()->size(), 6);

	ASSERT_EQ(ptr->size(), 6);
	ASSERT_FALSE(ptr == nullptr);

	std::unique_ptr<int> intNull = nullptr;
	ASSERT_THROW(extensions::not_null_ptr<std::unique_ptr<int>> x(std::move(intNull)), std::exception);
}

TEST_F(test_not_null_ptr, TestRawPtrInPlace)
{
	auto ptr = extensions::make_not_null_ptr<std::list<int>>(5);
	ASSERT_EQ((*ptr).size(), 5);
}

TEST_F(test_not_null_ptr, TestSharedPtrInPlace)
{
	auto ptr = extensions::make_not_null_ptr<std::shared_ptr<std::list<int>>>(5);
	ASSERT_EQ((*ptr).size(), 5);
}

TEST_F(test_not_null_ptr, TestUniquePtrInPlace)
{
	auto ptr = extensions::make_not_null_ptr<std::unique_ptr<std::list<int>>>(5);
	ASSERT_EQ((*ptr).size(), 5);
}

TEST_F(test_not_null_ptr, TestRawPtrCopySharesOwnership)
{
	int deletes = 0;
	{
		extensions::not_null_ptr<int*> ptr(new int(5), [&deletes](int* p)
			{
				delete p;
				deletes++;
			});
		{
			auto copy = ptr;
			ASSERT_EQ(*copy, 5);
		}
		ASSERT_EQ(deletes, 0);
		ASSERT_EQ(*ptr, 5);
	}
	ASSERT_EQ(deletes, 1);
}

TEST_F(test_not_null_ptr, TestRawPtrSwapTransfersOwnership)
{
	std::vector<int*> deleted;
	auto deleter = [&deleted](int* p)
	{
		deleted.push_back(p);
		delete p;
	};

	extensions::not_null_ptr<int*> first(new int(1), deleter);
	{
		extensions::not_null_ptr<int*> second(new int(2), deleter);
		swap(first, second);
	}

	ASSERT_EQ(deleted.size(), size_t{ 1 });
	ASSERT_NE(deleted.front(), first.get());
	ASSERT_EQ(*first, 2);
}

TEST_F(test_not_null_ptr, TestSharedPtrMoveKeepsSourceNotNull)
{
	extensions::not_null_ptr<std::shared_ptr<int>> source(std::make_shared<int>(5));
	auto target = std::move(source);

	ASSERT_NE(source.get(), nullptr);
	ASSERT_EQ(*source, 5);
	ASSERT_EQ(*target, 5);
}

TEST_F(test_not_null_ptr, TestComparison)
{
	auto first = extensions::make_not_null_ptr<std::shared_ptr<int>>(1);
	auto second = first;
	auto third = extensions::make_not_null_ptr<std::shared_ptr<int>>(1);

	ASSERT_TRUE(first == second);
	ASSERT_FALSE(first == third);
	ASSERT_TRUE(first < third || third < first);
}

TEST_F(test_not_null_ptr, TestSharedPtrInPlaceWithoutArguments)
{
	auto ptr = extensions::make_not_null_ptr<std::shared_ptr<std::list<int>>>();
	ASSERT_TRUE(ptr->empty());
}

TEST_F(test_not_null_ptr, TestUniquePtrIsNotMovable)
{
	static_assert(!std::is_move_constructible_v<extensions::not_null_ptr<std::unique_ptr<int>>>);
	static_assert(!std::is_move_assignable_v<extensions::not_null_ptr<std::unique_ptr<int>>>);
}

} // namespace framework_tests
