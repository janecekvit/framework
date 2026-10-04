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
	std::future<void> AddTask(std::function<void()>&& task)
	{
		auto future = std::async(std::launch::async, [task = std::move(task)]()
			{
				task();
			});

		std::this_thread::yield();
		return future;
	}

	template <typename T, bool R>
	void WaitForChange(synchronization::signal<T, R>& signal)
	{
		const auto oldSignalNumber = signal.get_signal_version();
		while (signal.is_signalized() && oldSignalNumber == signal.get_signal_version())
			std::this_thread::yield();
	}

	void WaitForTasks(std::list<std::reference_wrapper<std::future<void>>> tasks)
	{
		for (auto&& task : tasks)
			task.get().get();
	}
};

TEST_F(test_signal, PersistentState_ConditionVariable)
{
	synchronization::signal<std::condition_variable_any> s;
	std::atomic<int> woke_up_counter = 0;

	s.signalize();

	ASSERT_TRUE(s.is_signalized());
	ASSERT_EQ(1, s.get_signal_version());

	auto task1 = AddTask([&]()
		{
			std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
			s.wait(mtxQueueLock);
			woke_up_counter.fetch_add(1, std::memory_order_acq_rel);
		});

	task1.get();

	ASSERT_FALSE(s.is_signalized());
	ASSERT_EQ(1, s.get_signal_version());

	// result
	ASSERT_EQ(1, woke_up_counter.load());
}

TEST_F(test_signal, PersistentState_Semaphore)
{
	synchronization::signal<std::binary_semaphore> s;
	std::atomic<int> woke_up_counter = 0;

	s.signalize();

	ASSERT_TRUE(s.is_signalized());
	ASSERT_EQ(1, s.get_signal_version());

	auto task1 = AddTask([&]()
		{
			s.wait();
			woke_up_counter.fetch_add(1, std::memory_order_acq_rel);
		});

	task1.get();

	ASSERT_FALSE(s.is_signalized());
	ASSERT_EQ(1, s.get_signal_version());

	// result
	ASSERT_EQ(1, woke_up_counter.load());
}

TEST_F(test_signal, AutoReset_ConditionVariable)
{
	synchronization::signal<std::condition_variable_any> s;
	std::atomic<int> woke_up_counter = 0;
	auto callback = [&]()
	{
		std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
		s.wait(mtxQueueLock);
		woke_up_counter.fetch_add(1, std::memory_order_acq_rel);
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
	WaitForTasks({ task1, task2 });
	ASSERT_EQ(2, woke_up_counter.load());
}

TEST_F(test_signal, AutoReset_Semaphore)
{
	synchronization::signal<std::binary_semaphore> s;
	std::atomic<int> woke_up_counter = 0;
	auto callback = [&]()
	{
		s.wait();
		woke_up_counter.fetch_add(1, std::memory_order_acq_rel);
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
	WaitForTasks({ task1, task2 });
	ASSERT_EQ(2, woke_up_counter.load());
}

TEST_F(test_signal, ManualReset_ConditionVariable)
{
	synchronization::signal<std::condition_variable_any, true> s;
	std::atomic<int> woke_up_counter = 0;
	auto callback = [&]()
	{
		std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
		s.wait(mtxQueueLock);
		woke_up_counter.fetch_add(1, std::memory_order_acq_rel);
	};

	ASSERT_FALSE(s.is_signalized());
	ASSERT_EQ(0, s.get_signal_version());

	auto task1 = AddTask(callback);
	auto task2 = AddTask(callback);
	std::this_thread::sleep_for(100ms);

	s.signalize();
	WaitForTasks({ task1, task2 });

	ASSERT_EQ(1, s.get_signal_version());
	ASSERT_TRUE(s.is_signalized()); // still signalized - no manual reset

	// result
	ASSERT_EQ(2, woke_up_counter.load());
}

TEST_F(test_signal, ManualResetWithReset_ConditionVariable)
{
	synchronization::signal<std::condition_variable_any, true> s;
	std::atomic<int> woke_up_counter = 0;
	auto callback = [&]()
	{
		std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
		s.wait(mtxQueueLock);
		woke_up_counter.fetch_add(1, std::memory_order_acq_rel);
	};

	ASSERT_FALSE(s.is_signalized());
	ASSERT_EQ(0, s.get_signal_version());
	ASSERT_EQ(0, s.get_reset_version());

	auto task1 = AddTask(callback);

	s.signalize();
	WaitForTasks({ task1 });

	ASSERT_EQ(1, s.get_signal_version());
	ASSERT_TRUE(s.is_signalized()); // still signalized - no manual reset
	ASSERT_EQ(0, s.get_reset_version());

	// result
	ASSERT_EQ(1, woke_up_counter.load());

	// Reset sequence
	s.reset();

	ASSERT_EQ(1, s.get_signal_version());
	ASSERT_FALSE(s.is_signalized());
	ASSERT_EQ(1, s.get_reset_version());

	auto task2 = AddTask(callback);
	auto task3 = AddTask(callback);

	s.signalize();
	WaitForTasks({ task2, task3 });

	ASSERT_EQ(2, s.get_signal_version());
	ASSERT_TRUE(s.is_signalized());
	ASSERT_EQ(1, s.get_reset_version());

	// result
	ASSERT_EQ(3, woke_up_counter.load());
}

TEST_F(test_signal, SignalizeAll_ConditionVariable)
{
	synchronization::signal<std::condition_variable_any> s;
	std::atomic<int> woke_up_counter = 0;
	std::atomic<int> future_ready_counter = 0;
	auto callback = [&]()
	{
		std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
		future_ready_counter.fetch_add(1, std::memory_order_acq_rel);
		s.wait(mtxQueueLock);
		woke_up_counter.fetch_add(1, std::memory_order_acq_rel);
	};

	ASSERT_FALSE(s.is_signalized());
	ASSERT_EQ(0, s.get_signal_version());

	auto task1 = AddTask(callback);
	auto task2 = AddTask(callback);

	while (future_ready_counter.load(std::memory_order_acquire) != 2)
		std::this_thread::yield();

	{
		std::lock_guard<std::mutex> lock(m_conditionMtx); // both waiters released the lock inside wait
	}

	s.signalize_all();

	ASSERT_EQ(1, s.get_signal_version());
	ASSERT_FALSE(s.is_signalized());

	WaitForTasks({ task1, task2 });

	// result
	ASSERT_EQ(2, woke_up_counter.load());
}

TEST_F(test_signal, WaitPredicate_ConditionVariable)
{
	synchronization::signal<std::condition_variable_any> s;
	std::atomic<int> woke_up_counter = 0;

	s.signalize();

	ASSERT_TRUE(s.is_signalized());
	ASSERT_EQ(1, s.get_signal_version());

	{
		std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
		s.wait(mtxQueueLock, [&]()
			{
				woke_up_counter.fetch_add(1, std::memory_order_acq_rel);
				return true;
			});
	}

	ASSERT_TRUE(s.is_signalized()); // because predicate is true
	ASSERT_EQ(1, s.get_signal_version());

	auto task1 = AddTask([&]()
		{
			std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
			s.wait(mtxQueueLock, [&]()
				{
					woke_up_counter.fetch_add(1, std::memory_order_acq_rel);
					return false;
				});
		});

	task1.get();

	ASSERT_FALSE(s.is_signalized());
	ASSERT_EQ(1, s.get_signal_version());

	// result
	ASSERT_EQ(2, woke_up_counter.load());
}

TEST_F(test_signal, WaitPredicate_Semaphore)
{
	synchronization::signal<std::binary_semaphore> s;
	std::atomic<int> woke_up_counter = 0;

	s.signalize();

	ASSERT_TRUE(s.is_signalized());
	ASSERT_EQ(1, s.get_signal_version());

	s.wait([&]()
		{
			woke_up_counter.fetch_add(1, std::memory_order_acq_rel);
			return true;
		});

	ASSERT_TRUE(s.is_signalized()); // because predicate is true
	ASSERT_EQ(1, s.get_signal_version());

	auto task1 = AddTask([&]()
		{
			s.wait([&]()
				{
					woke_up_counter.fetch_add(1, std::memory_order_acq_rel);
					return false;
				});
		});

	task1.get();

	ASSERT_FALSE(s.is_signalized());
	ASSERT_EQ(1, s.get_signal_version());

	// result
	ASSERT_EQ(2, woke_up_counter.load());
}

TEST_F(test_signal, WaitForPredicate_ConditionVariable)
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

TEST_F(test_signal, WaitForPredicate_Semaphore)
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

TEST_F(test_signal, WaitUntilPredicate_ConditionVariable)
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

TEST_F(test_signal, WaitUntilPredicate_Semaphore)
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

TEST_F(test_signal, NoLostWakeup_ConditionVariable)
{
	auto runRounds = [](auto& s, std::mutex& mtx)
	{
		for (int round = 0; round < 20000; round++)
		{
			auto waiter = std::async(std::launch::async, [&]()
				{
					std::unique_lock<std::mutex> lock(mtx);
					s.wait(lock);
				});

			s.signalize();
			if (waiter.wait_for(5s) != std::future_status::ready)
			{
				s.signalize();
				return false;
			}
		}
		return true;
	};

	synchronization::signal<std::condition_variable_any> anySignal;
	ASSERT_TRUE(runRounds(anySignal, m_conditionMtx));

	synchronization::signal<std::condition_variable> plainSignal;
	ASSERT_TRUE(runRounds(plainSignal, m_conditionMtx));
}

TEST_F(test_signal, SignalizeTwice_Semaphore)
{
	synchronization::signal<std::binary_semaphore> s;
	s.signalize();
	s.signalize();

	s.wait();
	ASSERT_FALSE(s.wait_for(10ms));
}

TEST_F(test_signal, ManualResetDoesNotReleaseWaiter_ConditionVariable)
{
	synchronization::signal<std::condition_variable_any, true> s;
	std::atomic<bool> waiting = false;
	auto task = std::async(std::launch::async, [&]()
		{
			std::unique_lock<std::mutex> lock(m_conditionMtx);
			waiting = true;
			return s.wait_for(lock, 200ms);
		});

	while (!waiting)
		std::this_thread::yield();

	{
		std::lock_guard<std::mutex> lock(m_conditionMtx);
	}

	s.reset();
	ASSERT_FALSE(task.get());
}

TEST_F(test_signal, ManualResetSignalizeThenResetReleasesWaiter_ConditionVariable)
{
	synchronization::signal<std::condition_variable_any, true> s;
	std::atomic<bool> waiting = false;
	auto task = std::async(std::launch::async, [&]()
		{
			std::unique_lock<std::mutex> lock(m_conditionMtx);
			waiting = true;
			s.wait(lock);
		});

	while (!waiting)
		std::this_thread::yield();

	{
		std::lock_guard<std::mutex> lock(m_conditionMtx);
	}

	s.signalize();
	s.reset();

	const auto status = task.wait_for(5s);
	if (status != std::future_status::ready)
		s.signalize();

	ASSERT_EQ(std::future_status::ready, status);
}

TEST_F(test_signal, LvaluePredicateIsNotMoved_ConditionVariable)
{
	synchronization::signal<std::condition_variable_any> s;
	std::unique_lock<std::mutex> lock(m_conditionMtx);
	auto expected = "predicate state that does not fit into small string buffer"s;
	auto predicate = [expected]()
	{
		return !expected.empty();
	};

	s.wait(lock, predicate);
	ASSERT_TRUE(predicate());
}

} // namespace framework_tests
