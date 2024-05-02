#include "stdafx.h"

#include "CppUnitTest.h"
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

class IParamTest
{
public:
	virtual ~IParamTest()								   = default;
	virtual void Run(_In_ storage::parameter_pack&& oPack) = 0;
};

ONLY_USED_AT_NAMESPACE_SCOPE class test_parameter_pack : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_parameter_pack> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
public:
#ifdef __cpp_lib_any
	TEST_METHOD(TestParameterPackCxx17)
	{
		class CParamTest : public virtual IParamTest
		{
		public:
			CParamTest()		  = default;
			virtual ~CParamTest() = default;
			virtual void Run(_In_ storage::parameter_pack&& oPack) override
			{
				// Use unpack by return tuple
				auto [iNumber1, iNumber2, pNumber, pShared, pInterface] = oPack.get_pack<int, int, int*, std::shared_ptr<int>, CInterface*>();

				Assert::AreEqual(iNumber1, 25);
				Assert::AreEqual(iNumber2, 333);
				Assert::AreEqual(*pNumber, 666);
				Assert::AreEqual(*pShared, 777);
				Assert::AreEqual(pInterface->Do(), 1111);

				a = iNumber1;
				b = iNumber2;
				c = pNumber;
				d = pShared;
			}

		public:
			int a				   = 0;
			int b				   = 0;
			int* c				   = nullptr;
			std::shared_ptr<int> d = nullptr;
			std::unique_ptr<int> e = nullptr;
		};

		CParamTest oTest;

		int* pInt	 = new int(666);
		auto pShared = std::make_shared<int>(777);
		CInterface oInt;

		// Initialize parameter pack
		auto oPack = storage::parameter_pack(25, 333, pInt, pShared, &oInt);
		oTest.Run(std::move(oPack));

		Assert::AreEqual(oTest.a, 25);
		Assert::AreEqual(oTest.b, 333);
		Assert::AreEqual(*oTest.c, 666);
		Assert::AreEqual(*oTest.d, 777);
	}
#else
	TEST_METHOD(TestParameterPackCxx11)
	{
		class CParamTest : public virtual IParamTest
		{
		public:
			CParamTest()		  = default;
			virtual ~CParamTest() = default;
			virtual void Run(_In_ storage::parameter_pack&& oPack) override
			{
				CInterface* pInt = nullptr;

				// Use unpack by output parameters
				oPack.get_pack(a, b, c, d, pInt);
				Assert::AreEqual(a, 25);
				Assert::AreEqual(b, 333);
				Assert::AreEqual(*c, 666);
				Assert::AreEqual(*d, 777);
				Assert::AreEqual(pInt->Do(), 1111);
			}

		public:
			int a				   = 0;
			int b				   = 0;
			int* c				   = nullptr;
			std::shared_ptr<int> d = nullptr;
		};

		CParamTest oTest;

		int* pInt	 = new int(666);
		auto pShared = std::make_shared<int>(777);
		CInterface oInt;

		// Initialize parameter pack
		auto oPack = storage::parameter_pack(25, 333, pInt, pShared, &oInt);
		oTest.Run(std::move(oPack));

		Assert::AreEqual(oTest.a, 25);
		Assert::AreEqual(oTest.b, 333);
		Assert::AreEqual(*oTest.c, 666);
		Assert::AreEqual(*oTest.d, 777);
	}

#endif
};
} // namespace FrameworkTesting
