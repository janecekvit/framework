#include "stdafx.h"

#include "CppUnitTest.h"
#include "synchronization/signal.h"

#include <future>
#include <semaphore>
#include <shared_mutex>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace janecekvit;
using namespace std::string_literals;
using namespace std::chrono_literals;

namespace FrameworkTesting
{

ONLY_USED_AT_NAMESPACE_SCOPE class test_signal : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_signal> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
public:
	std::mutex m_conditionMtx;
	std::shared_mutex m_testMtx;

	void CompareStatuses(const std::list<std::future_status>& actual, const std::list<std::future_status>& expected)
	{
		Assert::AreEqual(actual.size(), expected.size());
		auto itExpected = expected.begin();
		for (const auto& status : actual)
		{
			Assert::AreEqual(static_cast<size_t>(*itExpected), static_cast<size_t>(status));
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
		Assert::IsTrue(bSetClalled);
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
		Assert::IsTrue(bSetClalled);
	}

	TEST_METHOD(ScenarioConditionVariableSetBeforeWait)
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
				std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
				using namespace std::chrono_literals;
				status = acv.wait_for(mtxQueueLock, 100ms) == std::cv_status::no_timeout;
			});

		// status is false because the condition variable was set before the wait
		// and there is not any mechanism to wake-up the thread that waiting after notification
		Assert::IsFalse(status);
	}

	TEST_METHOD(ScenarioConditionVariableSetAfterWait)
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
		Assert::IsTrue(status);
	}

	TEST_METHOD(ScenarioSemaphoreSetBeforeWait)
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

		Assert::IsTrue(status);
	}

	TEST_METHOD(ScenarioSemaphorSetAfterWait)
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

		Assert::IsTrue(status);
	}

	TEST_METHOD(ScenarioSignalConSetBeforeWait)
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

		Assert::IsTrue(status);
	}

	TEST_METHOD(ScenarioSignalConSetAfterWait)
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

		Assert::IsTrue(status);
	}

	TEST_METHOD(ScenarioSignalSemSetBeforeWait)
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

		Assert::IsTrue(status);
	}

	TEST_METHOD(ScenarioSignalSemSetAfterWait)
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

		Assert::IsTrue(status);
	}

	TEST_METHOD(SignalCon_Signalization)
	{
		int counter = 0;
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
		statuses.emplace_back(task1.wait_for(25ms));

		task1.get();

		CompareStatuses(statuses, { std::future_status::timeout, std::future_status::ready });
		Assert::AreEqual(1, counter);
	}

	TEST_METHOD(SignalCon_SignalizationAutoReset)
	{
		int counter = 0;
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

		Assert::AreEqual(2, counter);
	}

	TEST_METHOD(SignalCon_SignalizationManualReset)
	{
		int counter = 0;
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
		statuses.emplace_back(task1.wait_for(25ms));
		statuses.emplace_back(task2.wait_for(25ms));

		CompareStatuses(statuses, { std::future_status::timeout, std::future_status::timeout,
									  std::future_status::ready, std::future_status::ready });

		Assert::AreEqual(2, counter);
	}

	TEST_METHOD(SignalCon_SignalizationManualResetWithReset)
	{
		int counter = 0;
		std::list<std::future_status> statuses;
		synchronization::signal<std::condition_variable_any, true> s;
		auto callback = [&]()
		{
			std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
			s.wait(mtxQueueLock);
			counter++;
		};

		auto task1 = AddTask(callback);

		statuses.emplace_back(task1.wait_for(25ms));
		s.signalize();
		statuses.emplace_back(task1.wait_for(25ms));
		s.reset();

		auto task2 = AddTask(callback);

		statuses.emplace_back(task2.wait_for(25ms));
		s.signalize();
		statuses.emplace_back(task2.wait_for(25ms));

		CompareStatuses(statuses, { std::future_status::timeout, std::future_status::ready,
									  std::future_status::timeout, std::future_status::ready });

		Assert::AreEqual(2, counter);
	}

	TEST_METHOD(SignalCon_WaitPredicate)
	{
		int counter = 0;
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
		Assert::AreEqual(1, counter);
		s.signalize();
		statuses.emplace_back(task1.wait_for(25ms));
		Assert::AreEqual(2, counter);

		CompareStatuses(statuses, { std::future_status::timeout, std::future_status::ready });
	}

	TEST_METHOD(SignalCon_WaitForPredicate)
	{
		synchronization::signal<std::condition_variable_any> s;
		std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);

		Assert::IsTrue(s.wait_for(mtxQueueLock, 0ms, []()
			{
				return true;
			}));

		Assert::IsFalse(s.wait_for(mtxQueueLock, 0ms));

		s.signalize();
		Assert::IsTrue(s.wait_for(mtxQueueLock, 0ms));
	}

	TEST_METHOD(SignalCon_WaitUntilPredicate)
	{
		synchronization::signal<std::condition_variable_any> s;
		std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);

		Assert::IsTrue(s.wait_until(mtxQueueLock, std::chrono::steady_clock::now(), []()
			{
				return true;
			}));

		Assert::IsFalse(s.wait_until(mtxQueueLock, std::chrono::steady_clock::now()));

		s.signalize();
		Assert::IsTrue(s.wait_until(mtxQueueLock, std::chrono::steady_clock::now()));
	}

	TEST_METHOD(SignalSem_Signalization)
	{
		int counter = 0;
		std::list<std::future_status> statuses;
		synchronization::signal<std::binary_semaphore> s;

		auto task1 = AddTask([&]()
			{
				s.wait();
				counter++;
			});

		statuses.emplace_back(task1.wait_for(0ms));
		s.signalize();
		statuses.emplace_back(task1.wait_for(25ms));

		task1.get();

		CompareStatuses(statuses, { std::future_status::timeout, std::future_status::ready });
		Assert::AreEqual(1, counter);
	}

	TEST_METHOD(SignalSem_SignalizationAutoReset)
	{
		int counter = 0;
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
		statuses.emplace_back(task1.wait_for(25ms));
		statuses.emplace_back(task2.wait_for(0ms));
		s.signalize();
		statuses.emplace_back(task1.wait_for(0ms));
		statuses.emplace_back(task2.wait_for(25ms));

		CompareStatuses(statuses, { std::future_status::timeout, std::future_status::timeout,
									  std::future_status::ready, std::future_status::timeout,
									  std::future_status::ready, std::future_status::ready });

		Assert::AreEqual(2, counter);
	}

	TEST_METHOD(SignalSem_SignalizationManualReset)
	{
		int counter = 0;
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
		statuses.emplace_back(task1.wait_for(25ms));
		statuses.emplace_back(task2.wait_for(25ms));

		CompareStatuses(statuses, { std::future_status::timeout, std::future_status::timeout,
									  std::future_status::ready, std::future_status::ready });

		Assert::AreEqual(2, counter);
	}

	TEST_METHOD(SignalSem_SignalizationManualResetWithReset)
	{
		int counter = 0;
		std::list<std::future_status> statuses;
		synchronization::signal<std::binary_semaphore, true> s;
		auto callback = [&]()
		{
			s.wait();
			counter++;
		};

		auto task1 = AddTask(callback);

		statuses.emplace_back(task1.wait_for(25ms));
		s.signalize();
		statuses.emplace_back(task1.wait_for(25ms));
		s.reset();

		auto task2 = AddTask(callback);

		statuses.emplace_back(task2.wait_for(25ms));
		s.signalize();
		statuses.emplace_back(task2.wait_for(25ms));

		CompareStatuses(statuses, { std::future_status::timeout, std::future_status::ready,
									  std::future_status::timeout, std::future_status::ready });

		Assert::AreEqual(2, counter);
	}

	TEST_METHOD(SignalSem_WaitPredicate)
	{
		int counter = 0;
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
		Assert::AreEqual(1, counter);
		s.signalize();
		statuses.emplace_back(task1.wait_for(25ms));
		Assert::AreEqual(2, counter);

		CompareStatuses(statuses, { std::future_status::timeout, std::future_status::ready });
	}

	TEST_METHOD(SignalSem_WaitForPredicate)
	{
		synchronization::signal<std::binary_semaphore> s;

		Assert::IsTrue(s.wait_for(0ms, []()
			{
				return true;
			}));

		Assert::IsFalse(s.wait_for(0ms));

		s.signalize();
		Assert::IsTrue(s.wait_for(0ms));
	}

	TEST_METHOD(SignalSem_WaitUntilPredicate)
	{
		synchronization::signal<std::binary_semaphore> s;

		Assert::IsTrue(s.wait_until(std::chrono::steady_clock::now(), []()
			{
				return true;
			}));

		Assert::IsFalse(s.wait_until(std::chrono::steady_clock::now()));

		s.signalize();
		Assert::IsTrue(s.wait_until(std::chrono::steady_clock::now()));
	}
};

} // namespace FrameworkTesting