#include "stdafx.h"

#include "CppUnitTest.h"
#include "synchronization/event.h"

#include <future>
#include <semaphore>
#include <shared_mutex>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace janecekvit;
using namespace std::string_literals;

namespace FrameworkTesting
{

ONLY_USED_AT_NAMESPACE_SCOPE class test_event : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_event> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
public:
	std::mutex m_conditionMtx;
	std::shared_mutex m_testMtx;

	void PrepareScenarioSetBeforeWait(std::function<void()>&& setCallback, std::function<void()>&& waitCallback)
	{
		bool bSetClalled = false;
		m_testMtx.lock();
		auto task = std::async(std::launch::async, [&, this]()
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
		auto task = std::async(std::launch::async, [&, this]()
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

	TEST_METHOD(TestConditionVariabletSetBeforeWait)
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

	TEST_METHOD(TestConditionVariabletSetAfterWait)
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

	TEST_METHOD(TestSemaphoreSetBeforeWait)
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

	TEST_METHOD(TestSemaphorSetAfterAquire)
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
				using namespace std::chrono_literals;
				status = sem.try_acquire_for(100ms);
			});

		Assert::IsTrue(status);
	}

	TEST_METHOD(TestEventSetBeforeWait)
	{
		synchronization::event e;
		auto status = false;

		PrepareScenarioSetBeforeWait(
			[&]()
			{
				e.notify_one();
			},
			[&]()
			{
				std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
				using namespace std::chrono_literals;
				status = e.wait_for(mtxQueueLock, 100ms);
			});

		Assert::IsTrue(status);
	}

	TEST_METHOD(TestEventSetAfterWait)
	{
		synchronization::event e;
		auto status = false;

		PrepareScenarioSetAfterWait(
			[&]()
			{
				e.notify_one();
			},
			[&]()
			{
				std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
				using namespace std::chrono_literals;
				status = e.wait_for(mtxQueueLock, 100ms);
			});

		Assert::IsTrue(status);
	}

	/*Assert::AreEqual(static_cast<size_t>(t), static_cast<size_t>(test::Warning));
	Assert::AreEqual(std::hash<std::thread::id>()(id), std::hash<std::thread::id>()(std::this_thread::get_id()));
	Assert::AreEqual(srcl.file_name(), defaultLocation.file_name());
	Assert::AreEqual(srcl.function_name(), "void __cdecl FrameworkTesting::test_trace::TestTrace(void)");
	Assert::AreEqual(srcl.line(), static_cast<uint_least32_t>(25));
	Assert::AreEqual(data, L"ANO: true"s);

	Assert::AreEqual(static_cast<size_t>(t2.priority()), static_cast<size_t>(test::Verbose));
	Assert::AreEqual(std::hash<std::thread::id>()(t2.thread_id()), std::hash<std::thread::id>()(std::this_thread::get_id()));
	Assert::AreEqual(t2.source_location().file_name(), defaultLocation.file_name());
	Assert::AreEqual(t2.source_location().function_name(), "void __cdecl FrameworkTesting::test_trace::TestTrace(void)");
	Assert::AreEqual(t2.source_location().line(), static_cast<uint_least32_t>(29));
	Assert::AreEqual(t2.data(), L"NE: false"s);*/
};

} // namespace FrameworkTesting