#include "stdafx.h"

#include "CppUnitTest.h"
#include "Thread/async_thread_pool.h"

#include <future>
#include <iostream>
#include <string>

using namespace janecekvit::thread;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FrameworkTesting
{

constexpr const size_t thread_size = 4;

ONLY_USED_AT_NAMESPACE_SCOPE class test_async_thread_pool : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_async_thread_pool>
{
public:
	TEST_METHOD(Create)
	{
		int counter = 0;

		auto result = async::create([&counter]()
			{
				counter++;
			});

		result.get();
		Assert::AreEqual(counter, 1);
	}

	/*TEST_METHOD(Create2)
	{
		int counter = 0;
		{
			async::create([&counter](int i)
				{
					counter = i;
				}, (int)2);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		Assert::AreEqual(counter, 1);
	}*/

	TEST_METHOD(CreateWithResult)
	{
		auto result = async::create([]()
			{
				return 5;
			});
		Assert::AreEqual(result.get(), 5);
	}

	TEST_METHOD(CreateWithException)
	{
		auto task([]()
			{
				throw std::exception();
			});

		auto result = async::create(std::move(task));
		Assert::ExpectException<std::exception>([&result]()
			{
				result.get();
			});
	}
};
} // namespace FrameworkTesting
