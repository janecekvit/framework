#include "stdafx.h"

#include "CppUnitTest.h"
#include "Thread/sync_thread_pool.h"

#include <future>
#include <iostream>
#include <string>

using namespace janecekvit::thread;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FrameworkTesting
{

constexpr const size_t thread_size = 4;

ONLY_USED_AT_NAMESPACE_SCOPE class TestThread : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<TestThread>
{
public:
	TEST_METHOD(SyncThreadPoolSize)
	{
		sync_thread_pool pool(thread_size);
		Assert::AreEqual(pool.pool_size(), thread_size);
	}

	TEST_METHOD(SyncThreadSize)
	{
		sync_thread_pool pool(thread_size);
		pool.add_task(std::packaged_task<void()>([]
			{
			}));
		pool.add_task(std::packaged_task<void()>([]
			{
			}));
		Assert::AreEqual(pool.size(), size_t(2));
	}

	TEST_METHOD(SyncThreadAddTask)
	{
		int counter = 0;
		{
			sync_thread_pool pool(thread_size);

			std::packaged_task<void()> task([&counter]()
				{
					counter++;
				});

			pool.add_task(std::move(task));
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		Assert::AreEqual(counter, 1);
	}

	TEST_METHOD(SyncThreadAddTaskLambda)
	{
		int counter = 0;
		{
			sync_thread_pool pool(thread_size);
			pool.add_task([&counter]()
				{
					counter++;
				});
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		Assert::AreEqual(counter, 1);
	}

	TEST_METHOD(SyncThreadAddTaskWaitableTask)
	{
		int counter = 0;
		sync_thread_pool pool(thread_size);

		auto result = pool.add_waitable_task(std::packaged_task<void()>([&counter]
			{
				counter++;
			}));

		result.wait();
		Assert::AreEqual(counter, 1);
	}

	TEST_METHOD(SyncThreadAddTaskWaitableTaskWithResult)
	{
		sync_thread_pool pool(thread_size);
		auto result = pool.add_waitable_task(std::packaged_task<int()>([]()
			{
				return 5;
			}));
		Assert::AreEqual(result.get(), 5);
	}

	TEST_METHOD(SyncThreadAddTaskWaitableTaskWithResultLambda)
	{
		sync_thread_pool pool(thread_size);
		auto result = pool.add_waitable_task([]()
			{
				return 5;
			});
		Assert::AreEqual(result.get(), 5);
	}
};
} // namespace FrameworkTesting
