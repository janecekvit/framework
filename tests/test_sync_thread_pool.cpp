#include "Thread/sync_thread_pool.h"

#include <future>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

using namespace janecekvit::thread;

namespace framework_tests
{

constexpr const size_t thread_size = 4;

class test_sync_thread_pool : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}
};

TEST_F(test_sync_thread_pool, PoolSize)
{
	sync_thread_pool pool(thread_size);
	ASSERT_EQ(pool.pool_size(), thread_size);
}

TEST_F(test_sync_thread_pool, Size)
{
	std::promise<void> promise;
	auto future = promise.get_future().share();

	sync_thread_pool pool(thread_size);
	pool.add_task(std::packaged_task<void()>([future]
		{
			future.wait();
		}));

	pool.add_task(std::packaged_task<void()>([future]
		{
			future.wait();
		}));

	ASSERT_TRUE(pool.size() > 0); // could be 1 or more, because thread can already get the task from the queue

	promise.set_value();
}

TEST_F(test_sync_thread_pool, AddTask)
{
	std::atomic<int> counter = 0;
	{
		sync_thread_pool pool(thread_size);

		std::packaged_task<void()> task([&counter]()
			{
				counter++;
			});

		pool.add_task(std::move(task));
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	ASSERT_EQ(counter, 1);
}

TEST_F(test_sync_thread_pool, AddTaskLambda)
{
	std::atomic<int> counter = 0;
	{
		sync_thread_pool pool(thread_size);
		pool.add_task([&counter]()
			{
				counter++;
			});
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	ASSERT_EQ(counter, 1);
}

TEST_F(test_sync_thread_pool, AddWaitableTask)
{
	std::atomic<int> counter = 0;
	sync_thread_pool pool(thread_size);

	auto result = pool.add_waitable_task(std::packaged_task<void()>([&counter]
		{
			counter++;
		}));

	result.wait();
	ASSERT_EQ(counter, 1);
}

TEST_F(test_sync_thread_pool, AddWaitableTaskWithResult)
{
	sync_thread_pool pool(thread_size);
	auto result = pool.add_waitable_task(std::packaged_task<int()>([]()
		{
			return 5;
		}));
	ASSERT_EQ(result.get(), 5);
}

TEST_F(test_sync_thread_pool, AddWaitableTaskWithResultLambda)
{
	sync_thread_pool pool(thread_size);
	auto result = pool.add_waitable_task([]()
		{
			return 5;
		});
	ASSERT_EQ(result.get(), 5);
}

TEST_F(test_sync_thread_pool, AddWaitableTaskException)
{
	sync_thread_pool pool(thread_size);
	std::packaged_task<void()> task([]()
		{
			throw std::exception();
		});

	auto result = pool.add_waitable_task(std::move(task));
	ASSERT_THROW(result.get(), std::exception);
}

TEST_F(test_sync_thread_pool, AddMutipleTasks)
{
	constexpr int task_count = 1000;
	std::atomic<int> counter = 0;

	sync_thread_pool pool(thread_size);
	std::list<std::future<void>> results;

	for (size_t i = 0; i < task_count; i++)
	{
		results.emplace_back(pool.add_waitable_task(([&]()
			{
				counter++;
			})));
	}

	for (auto& result : results)
		result.wait();
	ASSERT_EQ(counter, task_count);
}
} // namespace framework_tests
