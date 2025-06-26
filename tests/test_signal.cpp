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
	void CompareStatuses(const std::list<std::future_status>& actual, const std::list<std::future_status>& expected)
	{
		ASSERT_EQ(actual.size(), expected.size());
		auto itExpected = expected.begin();
		for (const auto& status : actual)
		{
			ASSERT_EQ(static_cast<size_t>(*itExpected), static_cast<size_t>(status));
			++itExpected;
		}
	}

	std::future<void> AddTask(std::function<void()>&& task)
	{
		auto work = std::async(std::launch::async, [task = std::move(task)]()
			{
				task();
			});

		// replan scheduler
		std::this_thread::yield();
		work.wait_for(0ms);
		std::this_thread::yield();

		return work;
	}

	void PrepareScenarioSetBeforeWait(std::function<void()>&& setCallback, std::function<void()>&& waitCallback)
	{
		bool bSetClalled = false;
		m_testMtx.lock();
		auto task = AddTask([&, this]()
			{
				setCallback();
				bSetClalled = true;
				m_testMtx.unlock();
			});

		m_testMtx.lock();
		waitCallback();

		task.get();
		m_testMtx.unlock();
		ASSERT_TRUE(bSetClalled);
	}

	void PrepareScenarioSetAfterWait(std::function<void()>&& setCallback, std::function<void()>&& waitCallback)
	{
		bool bSetClalled = false;
		m_testMtx.lock();
		auto task = AddTask([&, this]()
			{
				std::unique_lock testLock(m_testMtx);
				setCallback();
				bSetClalled = true;
			});

		{
			m_testMtx.unlock();
			waitCallback();
		}

		task.get();
		ASSERT_TRUE(bSetClalled);
	}
};

TEST_F(test_signal, ScenarioConditionVariableSetBeforeWait)
{
	std::condition_variable acv;
	auto status = false;

	PrepareScenarioSetBeforeWait(
		[&]()
		{
			acv.notify_one();
		},
		[&]()
		{
			std::this_thread::yield();
			std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
			using namespace std::chrono_literals;
			status = acv.wait_for(mtxQueueLock, 10ms) == std::cv_status::no_timeout;
		});

	// status is false because the condition variable was set before the wait
	// and there is not any mechanism to wake-up the thread that waiting after notification
	ASSERT_FALSE(status);
}

TEST_F(test_signal, DISABLED_ScenarioConditionVariableSetAfterWait)
{
	auto status = false;
	std::condition_variable acv;

	PrepareScenarioSetAfterWait(
		[&]()
		{
			acv.notify_one();
		},
		[&]()
		{
			std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
			using namespace std::chrono_literals;
			status = acv.wait_for(mtxQueueLock, 100ms) == std::cv_status::no_timeout;
		});

	// status is true because the condition variable was set after the wait
	ASSERT_TRUE(status);
}

TEST_F(test_signal, ScenarioSemaphoreSetBeforeWait)
{
	auto status = false;
	std::binary_semaphore sem{ 0 };

	PrepareScenarioSetBeforeWait(
		[&]()
		{
			sem.release();
		},
		[&]()
		{
			using namespace std::chrono_literals;
			status = sem.try_acquire_for(100ms);
		});

	ASSERT_TRUE(status);
}

TEST_F(test_signal, ScenarioSemaphorSetAfterWait)
{
	std::binary_semaphore sem{ 0 };
	auto status = false;

	PrepareScenarioSetAfterWait(
		[&]()
		{
			sem.release();
		},
		[&]()
		{
			status = sem.try_acquire_for(100ms);
		});

	ASSERT_TRUE(status);
}

TEST_F(test_signal, ScenarioSignalConSetBeforeWait)
{
	auto status = false;
	synchronization::signal<std::condition_variable_any> s;

	PrepareScenarioSetBeforeWait(
		[&]()
		{
			s.signalize();
		},
		[&]()
		{
			std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
			status = s.wait_for(mtxQueueLock, 100ms);
		});

	ASSERT_TRUE(status);
}

TEST_F(test_signal, ScenarioSignalConSetAfterWait)
{
	auto status = false;
	synchronization::signal<std::condition_variable_any> s;

	PrepareScenarioSetAfterWait(
		[&]()
		{
			s.signalize();
		},
		[&]()
		{
			std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
			using namespace std::chrono_literals;
			status = s.wait_for(mtxQueueLock, 100ms);
		});

	ASSERT_TRUE(status);
}

TEST_F(test_signal, ScenarioSignalSemSetBeforeWait)
{
	auto status = false;
	synchronization::signal<std::binary_semaphore> s;

	PrepareScenarioSetBeforeWait(
		[&]()
		{
			s.signalize();
		},
		[&]()
		{
			status = s.wait_for(100ms);
		});

	ASSERT_TRUE(status);
}

TEST_F(test_signal, ScenarioSignalSemSetAfterWait)
{
	auto status = false;
	synchronization::signal<std::binary_semaphore> s;

	PrepareScenarioSetAfterWait(
		[&]()
		{
			s.signalize();
		},
		[&]()
		{
			status = s.wait_for(100ms);
		});

	ASSERT_TRUE(status);
}

TEST_F(test_signal, SignalCon_Signalization)
{
	std::atomic<int> counter = 0;
	std::list<std::future_status> statuses;
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

TEST_F(test_signal, SignalCon_SignalizationAutoReset)
{
	std::atomic<int> counter = 0;
	std::list<std::future_status> statuses;
	synchronization::signal<std::condition_variable_any> s;
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
	statuses.emplace_back(task1.wait_for(100ms));
	statuses.emplace_back(task2.wait_for(0ms));
	s.signalize();
	statuses.emplace_back(task1.wait_for(0ms));
	statuses.emplace_back(task2.wait_for(100ms));

	CompareStatuses(statuses, { std::future_status::timeout, std::future_status::timeout,
								  std::future_status::ready, std::future_status::timeout,
								  std::future_status::ready, std::future_status::ready });

	ASSERT_EQ(2, counter);
}

TEST_F(test_signal, DISABLED_SignalCon_SignalizationManualReset)
{
	std::atomic<int> counter = 0;
	std::list<std::future_status> statuses;
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
	std::list<std::future_status> statuses;
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
	std::list<std::future_status> statuses;
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

TEST_F(test_signal, SignalSem_Signalization)
{
	std::atomic<int> counter = 0;
	std::list<std::future_status> statuses;
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

TEST_F(test_signal, SignalSem_SignalizationAutoReset)
{
	std::atomic<int> counter = 0;
	std::list<std::future_status> statuses;
	synchronization::signal<std::binary_semaphore> s;
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
	statuses.emplace_back(task1.wait_for(100ms));
	statuses.emplace_back(task2.wait_for(0ms));
	s.signalize();
	statuses.emplace_back(task1.wait_for(0ms));
	statuses.emplace_back(task2.wait_for(100ms));

	CompareStatuses(statuses, { std::future_status::timeout, std::future_status::timeout,
								  std::future_status::ready, std::future_status::timeout,
								  std::future_status::ready, std::future_status::ready });

	ASSERT_EQ(2, counter);
}

TEST_F(test_signal, DISABLED_SignalSem_SignalizationManualReset)
{
	std::atomic<int> counter = 0;
	std::list<std::future_status> statuses;
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
	std::list<std::future_status> statuses;
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
	std::list<std::future_status> statuses;
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
