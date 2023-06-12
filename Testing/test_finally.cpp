#include "stdafx.h"

#include "CppUnitTest.h"
#include "extensions/finally.h"

using namespace janecekvit;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

////////////////////////////////////////////////////////////////////////////////////////////

namespace FrameworkTesting
{
ONLY_USED_AT_NAMESPACE_SCOPE class test_finally : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_finally> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
public:
#if (__cplusplus > __cpp_lib_concepts)
	TEST_METHOD(TestFinallyAction)
	{
		int result = 0;
		{
			auto finally = extensions::final_action([&]()
				{
					result = 5;
				});

			Assert::AreEqual(result, 0);
		}

		Assert::AreEqual(result, 5);
	}
	TEST_METHOD(TestFinallyActionWithException)
	{
		int result			 = 0;
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
					Assert::ExpectException<std::exception>([&]
						{
							bExceptionThrow = true;
							throw ex;
						});
				});

			Assert::AreEqual(result, 0);
		}

		Assert::AreEqual(result, 5); // value still on 5
		Assert::IsTrue(bExceptionThrow);
	}

	TEST_METHOD(TestFinallyMethod)
	{
		int result = 0;
		{
			auto finally = extensions::finally([&]()
				{
					result = 5;
				});

			Assert::AreEqual(result, 0);
		}

		Assert::AreEqual(result, 5);
	}
	TEST_METHOD(TestFinallyMethodWithException)
	{
		int result			 = 0;
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
					Assert::ExpectException<std::exception>([&]
						{
							bExceptionThrow = true;
							throw ex;
						});
				});

			Assert::AreEqual(result, 0);
		}

		Assert::AreEqual(result, 5); // value still on 5
		Assert::IsTrue(bExceptionThrow);
	}
#endif
};
} // namespace FrameworkTesting