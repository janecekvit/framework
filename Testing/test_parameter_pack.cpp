#include "stdafx.h"

#include <gtest/gtest.h>

#define __legacy
#include "storage/parameter_pack.h"

#include <fstream>
#include <iostream>

using namespace janecekvit;
using namespace std::string_literals;

namespace framework_tests
{
class interface
{
public:
	virtual ~interface() = default;
	virtual int do_something() = 0;
};

class derived : public virtual interface
{
public:
	derived() = default;
	virtual ~derived() = default;

	virtual int do_something() final override
	{
		return 1111;
	}
};

class test_parameter_pack : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}
};

TEST_F(test_parameter_pack, TestParameterPackCxx17)
{
	int* intPtr = new int(666);
	auto intShared = std::make_shared<int>(777);
	derived oInt;

	auto&& oPack = storage::parameter_pack(25, 333, intPtr, intShared, &oInt);
	auto&& [iNumber1, iNumber2, pNumber, pShared, pInterface] = oPack.get_pack<int, int, int*, std::shared_ptr<int>, derived*>();

	ASSERT_EQ(iNumber1, 25);
	ASSERT_EQ(iNumber2, 333);
	ASSERT_EQ(*pNumber, *intPtr);
	ASSERT_EQ(777, *pShared);

	delete intPtr;
}

TEST_F(test_parameter_pack, TestParameterPackCxx11)
{
	int* intPtr = new int(666);
	auto intShared = std::make_shared<int>(777);
	derived oInt;

	auto&& oPack = storage::parameter_pack_legacy(25, 333, intPtr, intShared, &oInt);

	int iNumber1, iNumber2;
	int* pNumber;
	std::shared_ptr<int> pShared;
	derived* pInt;
	oPack.get_pack(iNumber1, iNumber2, pNumber, pShared, pInt);

	ASSERT_EQ(iNumber1, 25);
	ASSERT_EQ(iNumber2, 333);
	ASSERT_EQ(*pNumber, *intPtr);
	ASSERT_EQ(777, *pShared);

	delete intPtr;
}
} // namespace framework_tests
