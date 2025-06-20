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

} // namespace framework_tests
