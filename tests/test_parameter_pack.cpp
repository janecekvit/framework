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

TEST_F(test_parameter_pack, TestNestedParameterPack)
{
	// This test verifies that copy/move constructors prevent infinite recursion
	// when parameter_pack is passed to another parameter_pack constructor

	auto pack1 = storage::parameter_pack(42, "hello"s, 3.14);
	ASSERT_EQ(pack1.size(), 3);

	auto pack2 = storage::parameter_pack(pack1);
	ASSERT_EQ(pack2.size(), 3);

	auto temp = storage::parameter_pack(100, 200);
	auto pack3 = storage::parameter_pack(std::move(temp));
	ASSERT_EQ(pack3.size(), 2);

	// verify that pack still contains original values
	auto&& [val1, val2, val3] = pack1.get_pack<int, std::string, double>();
	ASSERT_EQ(val1, 42);
	ASSERT_EQ(val2, "hello"s);
	ASSERT_DOUBLE_EQ(val3, 3.14);
}

TEST_F(test_parameter_pack, TestNestedParameterPackMove)
{
	auto temp = storage::parameter_pack(1, 2, 3, 4, 5);
	auto pack = std::move(temp);
	ASSERT_EQ(pack.size(), 5);
}

TEST_F(test_parameter_pack, TestNestedParameterPackCopy)
{
	auto pack1 = storage::parameter_pack(42, "hello"s, 3.14);
	auto pack2 = pack1;
	ASSERT_EQ(pack2.size(), 3);
}

TEST_F(test_parameter_pack, TestParameterPackCopySemantics)
{
	auto pack1 = storage::parameter_pack(10, 20, 30);
	auto pack2 = storage::parameter_pack(40, 50);
	pack2 = pack1; 
	ASSERT_EQ(pack2.size(), 3);

	auto&& [a, b, c] = pack2.get_pack<int, int, int>();
	ASSERT_EQ(a, 10);
	ASSERT_EQ(b, 20);
	ASSERT_EQ(c, 30);
}
TEST_F(test_parameter_pack, TestParameterPackCopyMoveSemantics)
{
	auto pack3 = storage::parameter_pack("test"s, 99);
	auto pack4 = storage::parameter_pack(1);
	pack4 = std::move(pack3);
	ASSERT_EQ(pack4.size(), 2);

	auto&& [str, num] = pack4.get_pack<std::string, int>();
	ASSERT_EQ(str, "test"s);
	ASSERT_EQ(num, 99);
}
} // namespace framework_tests
