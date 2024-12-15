#include "stdafx.h"

#include "CppUnitTest.h"

#define __legacy
#include "storage/parameter_pack.h"

#include <fstream>
#include <iostream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace janecekvit;
using namespace std::string_literals;

namespace FrameworkTesting
{
class IInterface
{
public:
	virtual ~IInterface() = default;
	virtual int Do()	  = 0;
};

class CInterface : public virtual IInterface
{
public:
	CInterface()		  = default;
	virtual ~CInterface() = default;
	virtual int Do() final override
	{
		return 1111;
	}
};

ONLY_USED_AT_NAMESPACE_SCOPE class test_parameter_pack : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_parameter_pack> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
public:
	TEST_METHOD(TestParameterPackCxx17)
	{
		int* intPtr	 = new int(666);
		auto intShared = std::make_shared<int>(777);
		CInterface oInt;

		auto&& oPack = storage::parameter_pack(25, 333, intPtr, intShared, &oInt);
		auto&& [iNumber1, iNumber2, pNumber, pShared, pInterface] = oPack.get_pack<int, int, int*, std::shared_ptr<int>, CInterface*>();

		Assert::AreEqual(iNumber1, 25);
		Assert::AreEqual(iNumber2, 333);
		Assert::AreEqual(*pNumber, *intPtr);
		Assert::AreEqual(777, *pShared);

		delete intPtr;
	}
	TEST_METHOD(TestParameterPackCxx11)
	{
		int* intPtr = new int(666);
		auto intShared = std::make_shared<int>(777);
		CInterface oInt;

		auto&& oPack = storage::parameter_pack_legacy(25, 333, intPtr, intShared, &oInt);
		
		int iNumber1, iNumber2;
		int* pNumber;
		std::shared_ptr<int> pShared;
		CInterface* pInt;
		oPack.get_pack(iNumber1, iNumber2, pNumber, pShared, pInt);

		Assert::AreEqual(iNumber1, 25);
		Assert::AreEqual(iNumber2, 333);
		Assert::AreEqual(*pNumber, *intPtr);
		Assert::AreEqual(777, *pShared);

		delete intPtr;
	}
};
} // namespace FrameworkTesting
