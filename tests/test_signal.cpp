#include "synchronization/signal.h"

#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <latch>
#include <semaphore>
#include <shared_mutex>

using namespace janecekvit;
using namespace std::string_literals;
using namespace std::chrono_literals;

namespace framework_tests
{

class test_signal : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}

	std::mutex m_conditionMtx;
	std::shared_mutex m_testMtx;

public:
	void CompareStatuses(const std::vector<std::future_status>& actual, const std::vector<std::future_status>& expected)
	{
		ASSERT_EQ(actual.size(), expected.size());
		for (size_t i = 0; i < actual.size(); ++i)
		{
			EXPECT_EQ(static_cast<size_t>(actual[i]), static_cast<size_t>(expected[i]))
				<< "Status mismatch at index " << i;
		}
	}

	std::future<void> AddTask(std::function<void()>&& task)
	{
		return std::async(std::launch::async, [task = std::move(task)]()
			{
				task();
			});
	}

	template <typename Rep, typename Period>
	std::future<void> AddTask(std::function<void()>&& task,
		std::chrono::duration<Rep, Period> delay)
	{
		return std::async(std::launch::async, [task = std::move(task), delay]()
			{
				if (delay > std::chrono::duration<Rep, Period>::zero())
				{
					std::this_thread::sleep_for(delay);
				}
				task();
			});
	}

	template <typename T>
	void WaitForChange(synchronization::signal<T>& signal)
	{
		const auto oldSignalNumber = signal.get_signal_version();
		while (signal.is_signalized() && oldSignalNumber == signal.get_signal_version())
			std::this_thread::yield();
	}
};

TEST_F(test_signal, AutoReset_ConditionVariable)
{
	synchronization::signal<std::condition_variable_any> s;
	std::atomic<int> counter = 0;
	auto callback = [&]()
	{
		std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
		s.wait(mtxQueueLock);
		counter.fetch_add(1, std::memory_order_acq_rel);
	};

	ASSERT_FALSE(s.is_signalized());
	ASSERT_EQ(0, s.get_signal_version());

	auto task1 = AddTask(callback);
	auto task2 = AddTask(callback);

	s.signalize();
	WaitForChange(s);

	ASSERT_EQ(1, s.get_signal_version());
	ASSERT_FALSE(s.is_signalized());

	s.signalize();
	WaitForChange(s);

	ASSERT_EQ(2, s.get_signal_version());
	ASSERT_FALSE(s.is_signalized());

	// result
	ASSERT_EQ(2, counter.load());
}

TEST_F(test_signal, AutoReset_Signal)
{
	synchronization::signal<std::binary_semaphore> s;
	std::atomic<int> counter = 0;
	auto callback = [&]()
	{
		s.wait();
		counter.fetch_add(1, std::memory_order_acq_rel);
	};

	ASSERT_FALSE(s.is_signalized());
	ASSERT_EQ(0, s.get_signal_version());

	auto task1 = AddTask(callback);
	auto task2 = AddTask(callback);

	s.signalize();
	WaitForChange(s);

	ASSERT_EQ(1, s.get_signal_version());
	ASSERT_FALSE(s.is_signalized());

	s.signalize();
	WaitForChange(s);

	ASSERT_EQ(2, s.get_signal_version());
	ASSERT_FALSE(s.is_signalized());

	// result
	ASSERT_EQ(2, counter.load());
}

TEST_F(test_signal, SignalCon_Signalization)
{
	std::atomic<int> counter = 0;
	std::vector<std::future_status> statuses;
	synchronization::signal<std::condition_variable_any> s;

	auto task1 = AddTask([&]()
		{
			std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
			s.wait(mtxQueueLock);
			counter++;
		});

	statuses.emplace_back(task1.wait_for(0ms));
	s.signalize();
	statuses.emplace_back(task1.wait_for(1s));

	task1.get();

	CompareStatuses(statuses, { std::future_status::timeout, std::future_status::ready });
	ASSERT_EQ(1, counter);
}

TEST_F(test_signal, SignalCon_SignalizationManualReset)
{
	std::atomic<int> counter = 0;
	std::vector<std::future_status> statuses;
	synchronization::signal<std::condition_variable_any, true> s;
	auto callback = [&]()
	{
		std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
		s.wait(mtxQueueLock);
		counter++;
	};

	auto task1 = AddTask(callback);
	auto task2 = AddTask(callback);

	statuses.emplace_back(task1.wait_for(0ms));
	statuses.emplace_back(task2.wait_for(0ms));
	s.signalize();
	statuses.emplace_back(task1.wait_for(1s));
	statuses.emplace_back(task2.wait_for(1s));

	CompareStatuses(statuses, { std::future_status::timeout, std::future_status::timeout,
								  std::future_status::ready, std::future_status::ready });

	ASSERT_EQ(2, counter);
}

TEST_F(test_signal, SignalCon_SignalizationManualResetWithReset)
{
	std::atomic<int> counter = 0;
	std::vector<std::future_status> statuses;
	synchronization::signal<std::condition_variable_any, true> s;
	auto callback = [&]()
	{
		std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
		s.wait(mtxQueueLock);
		counter++;
	};

	auto task1 = AddTask(callback);

	statuses.emplace_back(task1.wait_for(0ms));
	s.signalize();
	statuses.emplace_back(task1.wait_for(1s));
	s.reset();

	auto task2 = AddTask(callback);

	statuses.emplace_back(task2.wait_for(0ms));
	s.signalize();
	statuses.emplace_back(task2.wait_for(1s));

	CompareStatuses(statuses, { std::future_status::timeout, std::future_status::ready,
								  std::future_status::timeout, std::future_status::ready });

	ASSERT_EQ(2, counter);
}

TEST_F(test_signal, SignalCon_WaitPredicate)
{
	std::atomic<int> counter = 0;
	std::vector<std::future_status> statuses;
	synchronization::signal<std::condition_variable_any> s;
	auto callback = [&]()
	{
		std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
		s.wait(mtxQueueLock, []()
			{
				return true;
			});
		counter++;
		s.wait(mtxQueueLock, []()
			{
				return false;
			});
		counter++;
	};

	auto task1 = AddTask(callback);
	statuses.emplace_back(task1.wait_for(25ms));
	ASSERT_EQ(1, counter);
	s.signalize();
	statuses.emplace_back(task1.wait_for(1s));
	ASSERT_EQ(2, counter);

	CompareStatuses(statuses, { std::future_status::timeout, std::future_status::ready });
}

TEST_F(test_signal, SignalCon_WaitForPredicate)
{
	synchronization::signal<std::condition_variable_any> s;
	std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);

	ASSERT_TRUE(s.wait_for(mtxQueueLock, 0ms, []()
		{
			return true;
		}));

	ASSERT_FALSE(s.wait_for(mtxQueueLock, 0ms));

	s.signalize();
	ASSERT_TRUE(s.wait_for(mtxQueueLock, 0ms));
}

TEST_F(test_signal, SignalCon_WaitUntilPredicate)
{
	synchronization::signal<std::condition_variable_any> s;
	std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);

	ASSERT_TRUE(s.wait_until(mtxQueueLock, std::chrono::steady_clock::now(), []()
		{
			return true;
		}));

	ASSERT_FALSE(s.wait_until(mtxQueueLock, std::chrono::steady_clock::now()));

	s.signalize();
	ASSERT_TRUE(s.wait_until(mtxQueueLock, std::chrono::steady_clock::now()));
}

// TEST_F(test_signal, SignalCon_SignalizeAll)
//{
//	std::atomic<int> counter = 0;
//	std::vector<std::future_status> statuses;
//	synchronization::signal<std::condition_variable_any> s;
//	auto callback = [&]()
//	{
//		std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
//		s.wait(mtxQueueLock);
//		counter++;
//	};
//
//	auto task1 = AddTask(callback);
//	auto task2 = AddTask(callback);
//	auto task3 = AddTask(callback);
//
//	std::this_thread::sleep_for(500ms);
//	statuses.emplace_back(task1.wait_for(0ms));
//	statuses.emplace_back(task2.wait_for(0ms));
//	statuses.emplace_back(task3.wait_for(0ms));
//	s.signalize_all();
//	std::this_thread::sleep_for(500ms);
//	statuses.emplace_back(task1.wait_for(100ms));
//	statuses.emplace_back(task2.wait_for(100ms));
//	statuses.emplace_back(task3.wait_for(100ms));
//
//	CompareStatuses(statuses, { std::future_status::timeout, std::future_status::timeout, std::future_status::timeout,
//								  std::future_status::ready, std::future_status::ready, std::future_status::ready });
//
//	task1.get();
//	task2.get();
//	task3.get();
//
//	ASSERT_EQ(3, counter);
// }

TEST_F(test_signal, SignalSem_Signalization)
{
	std::atomic<int> counter = 0;
	std::vector<std::future_status> statuses;
	synchronization::signal<std::binary_semaphore> s;

	auto task1 = AddTask([&]()
		{
			s.wait();
			counter++;
		});

	statuses.emplace_back(task1.wait_for(0ms));
	s.signalize();
	statuses.emplace_back(task1.wait_for(1s));

	task1.get();

	CompareStatuses(statuses, { std::future_status::timeout, std::future_status::ready });
	ASSERT_EQ(1, counter);
}

TEST_F(test_signal, SignalSem_SignalizationManualReset)
{
	std::atomic<int> counter = 0;
	std::vector<std::future_status> statuses;
	synchronization::signal<std::binary_semaphore, true> s;
	auto callback = [&]()
	{
		s.wait();
		counter++;
	};

	auto task1 = AddTask(callback);
	auto task2 = AddTask(callback);

	statuses.emplace_back(task1.wait_for(0ms));
	statuses.emplace_back(task2.wait_for(0ms));
	s.signalize();
	statuses.emplace_back(task1.wait_for(1s));
	statuses.emplace_back(task2.wait_for(1s));

	CompareStatuses(statuses, { std::future_status::timeout, std::future_status::timeout,
								  std::future_status::ready, std::future_status::ready });

	ASSERT_EQ(2, counter);
}

TEST_F(test_signal, SignalSem_SignalizationManualResetWithReset)
{
	std::atomic<int> counter = 0;
	std::vector<std::future_status> statuses;
	synchronization::signal<std::binary_semaphore, true> s;
	auto callback = [&]()
	{
		s.wait();
		counter++;
	};

	auto task1 = AddTask(callback);

	statuses.emplace_back(task1.wait_for(0ms));
	s.signalize();
	statuses.emplace_back(task1.wait_for(1s));
	s.reset();

	auto task2 = AddTask(callback);

	statuses.emplace_back(task2.wait_for(0ms));
	s.signalize();
	statuses.emplace_back(task2.wait_for(1s));

	CompareStatuses(statuses, { std::future_status::timeout, std::future_status::ready,
								  std::future_status::timeout, std::future_status::ready });

	ASSERT_EQ(2, counter);
}

TEST_F(test_signal, SignalSem_WaitPredicate)
{
	std::atomic<int> counter = 0;
	std::vector<std::future_status> statuses;
	synchronization::signal<std::binary_semaphore> s;
	auto callback = [&]()
	{
		s.wait([]()
			{
				return true;
			});
		counter++;
		s.wait([]()
			{
				return false;
			});
		counter++;
	};

	auto task1 = AddTask(callback);
	statuses.emplace_back(task1.wait_for(25ms));
	ASSERT_EQ(1, counter);
	s.signalize();
	statuses.emplace_back(task1.wait_for(1s));
	ASSERT_EQ(2, counter);

	CompareStatuses(statuses, { std::future_status::timeout, std::future_status::ready });
}

TEST_F(test_signal, SignalSem_WaitForPredicate)
{
	synchronization::signal<std::binary_semaphore> s;

	ASSERT_TRUE(s.wait_for(0ms, []()
		{
			return true;
		}));

	ASSERT_FALSE(s.wait_for(0ms));

	s.signalize();
	ASSERT_TRUE(s.wait_for(0ms));
}

TEST_F(test_signal, SignalSem_WaitUntilPredicate)
{
	synchronization::signal<std::binary_semaphore> s;

	ASSERT_TRUE(s.wait_until(std::chrono::steady_clock::now(), []()
		{
			return true;
		}));

	ASSERT_FALSE(s.wait_until(std::chrono::steady_clock::now()));

	s.signalize();
	ASSERT_TRUE(s.wait_until(std::chrono::steady_clock::now()));
}

} // namespace framework_tests
