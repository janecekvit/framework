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

	std::future<void> AddTask(std::function<void()>&& task)
	{
		auto work = std::async(std::launch::async, [this, task = std::move(task)]()
			{
				task();
			});

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
		std::condition_variable acv;
		auto status = false;

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
		std::binary_semaphore sem{ 0 };
		auto status = false;

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
		synchronization::signal<std::condition_variable_any> s;
		auto status = false;

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
		synchronization::signal<std::condition_variable_any> s;
		auto status = false;

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
		synchronization::signal<std::binary_semaphore> s;
		auto status = false;

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
		synchronization::signal<std::binary_semaphore> s;
		auto status = false;

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
		synchronization::signal<std::condition_variable_any> s;
		int counter = 0;

		std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
		std::list<std::future<void>> tasks;
		tasks.emplace_back(AddTask([&]()
			{
				s.wait(mtxQueueLock);
				counter++;
			}));

		s.signalize();

		std::ranges::for_each(tasks, [](auto& task)
		{
				task.get();
			});
		Assert::AreEqual(1, counter);
	}

	/*TEST_METHOD(SignalCon_SignalizationAll)
	{
		synchronization::signal<std::condition_variable_any> s;
		int counter = 0;

		std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
		std::list<std::future<void>> tasks;
		tasks.emplace_back(AddTask([&]()
			{
				s.wait(mtxQueueLock);
				counter++;
			}));
		tasks.emplace_back(AddTask([&]()
			{
				s.wait(mtxQueueLock);
				counter++;
			}));
		tasks.emplace_back(AddTask([&]()
			{
				s.wait(mtxQueueLock);
				counter++;
			}));

		s.signalize();

		std::ranges::for_each(tasks, [](auto& task)
			{
				task.get();
			});
		Assert::AreEqual(3, counter);
	}*/
};

} // namespace FrameworkTesting