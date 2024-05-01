#include "stdafx.h"

#include "CppUnitTest.h"
#include "extensions/lazy.h"

using namespace janecekvit;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

////////////////////////////////////////////////////////////////////////////////////////////

namespace FrameworkTesting
{
ONLY_USED_AT_NAMESPACE_SCOPE class test_lazy : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_lazy> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
public:
	TEST_METHOD(TestLazyAction)
	{
		// value by capture
		int result		 = 0;
		auto lazy_action = extensions::lazy_action<void>([&]()
			{
				result = 5;
			});

		Assert::AreEqual(result, 0);
		lazy_action();
		Assert::AreEqual(result, 5);
	}

	TEST_METHOD(TestLazyActionReturnValue)
	{
		// return value
		int result	= 0;
		auto lambda = [&]()
		{
			return result;
		};
		auto lazy_action = extensions::lazy_action<int>(std::move(lambda));
		Assert::AreEqual(result, 0);
		result = 5;
		Assert::AreEqual(lazy_action(), 5);
	}

	TEST_METHOD(TestLazyActionParametersDefaultValues)
	{
		// default parameter value
		auto lambda = [&](int i, int j)
		{
			return i + j;
		};
		auto lazy_action = extensions::lazy_action<int, int, int>(std::move(lambda), 5, 6);
		Assert::AreEqual(lazy_action(), 11);
	}

	TEST_METHOD(TestLazyActionChangedValuesByParameter)
	{
		// update value by parameters value
		auto lambda = [&](int i, int j)
		{
			return i + j;
		};
		auto lazy_action = extensions::lazy_action<int, int, int>(std::move(lambda), 5, 6);
		Assert::AreEqual(lazy_action(5, 9), 14); // throw out old values
	}

	TEST_METHOD(TestLazyMethod)
	{
		// value by capture
		int result		 = 0;
		auto lazy_action = extensions::lazy_action<void>([&]()
			{
				result = 5;
			});

		Assert::AreEqual(result, 0);
		lazy_action();
		Assert::AreEqual(result, 5);
	}

	TEST_METHOD(TestLazyMethodReturnValue)
	{
		// return value
		int result	= 0;
		auto lambda = [&]()
		{
			return result;
		};
		auto lazy_action = extensions::lazy(std::move(lambda));
		Assert::AreEqual(result, 0);
		result = 5;
		Assert::AreEqual(lazy_action(), 5);
	}

	TEST_METHOD(TestLazyMethodParametersDefaultValues)
	{
		// default parameter value
		auto lambda = [&](int i, int j)
		{
			return i + j;
		};
		auto lazy_action = extensions::lazy(std::move(lambda), 5, 6);
		Assert::AreEqual(lazy_action(), 11);
	}

	TEST_METHOD(TestLazyMethodChangedValuesByParameter)
	{
		// update value by parameters value
		auto lambda = [&](int i, int j)
		{
			return i + j;
		};
		auto lazy_action = extensions::lazy(std::move(lambda), 5, 6);
		Assert::AreEqual(lazy_action(5, 9), 14); // throw out old values
	}
};
} // namespace FrameworkTesting