#include "stdafx.h"

#include "CppUnitTest.h"
#include "extensions/resource_wrapper.h"

#include <fstream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace janecekvit;
using namespace std::string_literals;

namespace FrameworkTesting
{
ONLY_USED_AT_NAMESPACE_SCOPE class test_resource_wrapper : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_resource_wrapper> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
public:
	TEST_METHOD(TestRelease)
	{
		int* iValueChecker = nullptr;
		{
			auto oWrapperInt = janecekvit::extensions::resource_wrapper<int*>(new int(5), [&](int*& i)
				{
					// check if we obtain same pointer through assignment,
					// if yes reset the value checker, so we knew that lambda is called during destruction
					if (iValueChecker == i)
						iValueChecker = nullptr;

					delete i;
					i = nullptr;
				});

			iValueChecker = oWrapperInt;
			Assert::IsNotNull(iValueChecker);
		}

		Assert::IsNull(iValueChecker);
	}

	TEST_METHOD(TestUserConversion)
	{
		{
			auto oWrapperInt = janecekvit::extensions::resource_wrapper<int*>(new int(5), [](int*& i)
				{
					delete i;
					i = nullptr;
				});

			int& iReference		  = *oWrapperInt;
			int* iUserDefinedCast = oWrapperInt; // user defined conversion
			bool bTrue			  = oWrapperInt; // user defined conversion bool -> true

			Assert::IsNotNull(iUserDefinedCast);

			Assert::AreEqual(*iUserDefinedCast, 5);
			Assert::AreEqual(*oWrapperInt, 5);
			Assert::AreEqual(iReference, 5);
			Assert::IsTrue(bTrue);
		}
	}

	TEST_METHOD(TestReset)
	{
		int* iValueChecker = nullptr;
		{
			auto oWrapperInt = janecekvit::extensions::resource_wrapper<int*>(new int(5), [&](int*& i)
				{
					// check if we obtain same pointer through assignment,
					// if yes reset the value checker, so we knew that lambda is called during destruction
					if (iValueChecker == i)
						iValueChecker = nullptr;

					delete i;
					i = nullptr;
				});

			iValueChecker = oWrapperInt;
			Assert::IsNotNull(iValueChecker);

			// release resource before scope ends
			oWrapperInt.reset();

			Assert::IsNull(iValueChecker);
		}

		Assert::IsNull(iValueChecker);
	}

	TEST_METHOD(TestRetrieve)
	{
		auto oWrapperList = janecekvit::extensions::resource_wrapper<std::list<int>>(std::list<int>{ 1, 2 }, [](std::list<int>& i)
			{
				i.clear();
			});

		Assert::AreEqual(size_t{ 2 }, oWrapperList->size());

		bool bRetrieveCalled = false;
		oWrapperList.retrieve([&](auto&& list)
			{
				Assert::AreEqual(size_t{ 2 }, oWrapperList->size());
				bRetrieveCalled = true;
			});

		Assert::IsTrue(bRetrieveCalled);
	}

	TEST_METHOD(TestUpdate)
	{
		auto oWrapperList = janecekvit::extensions::resource_wrapper<std::list<int>>(std::list<int>{ 1, 2 }, [](std::list<int>& i)
			{
				i.clear();
			});

		Assert::AreEqual(size_t{ 2 }, oWrapperList->size());

		bool bRetrieveCalled = false;
		oWrapperList.update([](auto&& list)
			{
				list.emplace_back(3);
			});

		Assert::AreEqual(size_t{ 3 }, oWrapperList->size());
	}

	TEST_METHOD(TestContainers)
	{
		{ // Copy constructible
			auto oWrapperList = janecekvit::extensions::resource_wrapper<std::list<int>>(std::list<int>{ 10, 20, 30 }, [](std::list<int>& i)
				{
					i.clear();
				});

			Assert::AreEqual(size_t{ 3 }, oWrapperList->size());

			// No [[nodiscard]] attribute
			auto&& it  = oWrapperList.begin();
			auto&& it2 = oWrapperList.end();

			Assert::AreEqual(10, *it);
			Assert::AreEqual(30, *it2);

			Assert::AreEqual(size_t{ 30 }, oWrapperList->size());
		}
	}

	TEST_METHOD(TestReassignment)
	{
		int uLambdaCalled = 0;

		{ // wrapper scope -> copy constructor by assignment operator
			auto oWrapperInt = janecekvit::extensions::resource_wrapper<int*>(new int(5), [&uLambdaCalled](int*& i)
				{
					delete i;
					i = nullptr;
					uLambdaCalled++;
				});

			Assert::AreEqual(5, *oWrapperInt);

			// assignment
			oWrapperInt = new int(10);
			Assert::AreEqual(10, *oWrapperInt);
			Assert::AreEqual(1, uLambdaCalled);
		}

		Assert::AreEqual(2, uLambdaCalled);
	}

	TEST_METHOD(TestMoveRessignmentWithCopyDestruction)
	{
		int uLambdaCalled	  = 0;
		auto bCopyDestruction = false;
		{ // wrapper scope -> Copy constructor by assignment operator
			auto oWrappedInt1 = janecekvit::extensions::resource_wrapper<int*>(new int(5), [&uLambdaCalled](int*& i)
				{
					delete i;
					i = nullptr;
					uLambdaCalled++;
				});

			int* i = oWrappedInt1;
			Assert::AreEqual(*i, 5);
			Assert::AreEqual(*oWrappedInt1, 5);

			// Move re-assignment
			oWrappedInt1 = std::move(janecekvit::extensions::resource_wrapper<int*>(new int(10), [&uLambdaCalled](int*& i)
				{
					delete i;
					i = nullptr;
					uLambdaCalled++;
				}));

			Assert::AreEqual(*oWrappedInt1, 10);
			Assert::AreEqual(uLambdaCalled, 1);

			auto oWrappedInt2 = janecekvit::extensions::resource_wrapper<int*>(new int(15), [&uLambdaCalled, &bCopyDestruction](int*& i)
				{
					if (bCopyDestruction)
						return;

					delete i;
					i = nullptr;
					uLambdaCalled++;
					bCopyDestruction = true;
				});

			oWrappedInt1 = oWrappedInt2;
			Assert::AreEqual(*oWrappedInt1, 15);
			Assert::AreEqual(uLambdaCalled, 2);
			Assert::AreEqual(bCopyDestruction, false);
		}
		Assert::AreEqual(uLambdaCalled, 3);
		Assert::AreEqual(bCopyDestruction, true);
	}
};
} // namespace FrameworkTesting
