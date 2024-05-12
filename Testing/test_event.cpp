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

	TEST_METHOD(TestOrigConditionVariableNotifyOneBeforeWaitStarted)
	{
		bool bModified = false;
		std::condition_variable acv;
		auto status = std::cv_status::no_timeout;

		m_testMtx.lock();
		auto task = std::async(std::launch::async, [&, this]()
			{
				acv.notify_one();
				bModified = true;
				m_testMtx.unlock();
			});

		std::unique_lock testLock(m_testMtx);
		{
			std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
			using namespace std::chrono_literals;
			status = acv.wait_for(mtxQueueLock, 50ms);
		}

		task.get();
		m_testMtx.unlock();
		Assert::IsTrue(bModified);
		Assert::AreEqual(static_cast<size_t>(std::cv_status::timeout), static_cast<size_t>(status));
	}

	TEST_METHOD(TestOrigConditionVariableNotifyOneAfterWaitStarted)
	{
		bool bModified = false;
		std::condition_variable acv;
		auto status = std::cv_status::no_timeout;

		m_testMtx.lock();
		auto task = std::async(std::launch::async, [&, this]()
			{
				std::unique_lock testLock(m_testMtx);
				acv.notify_one();
				bModified = true;
			});

		{
			std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
			m_testMtx.unlock();
			using namespace std::chrono_literals;
			status = acv.wait_for(mtxQueueLock, 100ms);
		}

		task.get();
		Assert::IsTrue(bModified);
		Assert::AreEqual(static_cast<size_t>(std::cv_status::no_timeout), static_cast<size_t>(status));
	}

	TEST_METHOD(TestNotifyOneBeforeWaitStarted)
	{
		bool bModified = false;
		synchronization::event acv;
		auto status = false;

		m_testMtx.lock();
		auto task = std::async(std::launch::async, [&, this]()
			{
				acv.notify_one();
				bModified = true;
				m_testMtx.unlock();
			});

		std::unique_lock testLock(m_testMtx);
		{
			std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
			using namespace std::chrono_literals;
			status = acv.wait_for(mtxQueueLock, 100ms);
		}

		task.get();
		m_testMtx.unlock();
		Assert::IsTrue(bModified);
		Assert::IsTrue(status);
	}

	TEST_METHOD(TestNotifyOneAfterWaitStarted)
	{
		bool bModified = false;
		synchronization::event acv;
		auto status = false;

		m_testMtx.lock();
		auto task = std::async(std::launch::async, [&, this]()
			{
				std::unique_lock testLock(m_testMtx);
				acv.notify_one();
				bModified = true;
			});

		{
			std::unique_lock<std::mutex> mtxQueueLock(m_conditionMtx);
			m_testMtx.unlock();
			using namespace std::chrono_literals;
			status = acv.wait_for(mtxQueueLock, 100ms);
		}

		task.get();
		Assert::IsTrue(bModified);
		Assert::IsTrue(status);
	}

	TEST_METHOD(TestSemaphoreBeforeWaitStarted)
	{
		bool bModified = false;
		std::binary_semaphore sem{ 0 };
		auto status = false;

		m_testMtx.lock();
		auto task = std::async(std::launch::async, [&, this]()
			{
				sem.release();
				bModified = true;
				m_testMtx.unlock();
			});

		std::unique_lock testLock(m_testMtx);
		{
			using namespace std::chrono_literals;
			status = sem.try_acquire_for(100ms);
		}

		task.get();
		m_testMtx.unlock();
		Assert::IsTrue(bModified);
		Assert::IsTrue(status);
	}

	TEST_METHOD(TestSemaphorAfterAquireStarted)
	{
		bool bModified = false;
		std::binary_semaphore sem{ 0 };
		auto status = false;

		m_testMtx.lock();
		auto task = std::async(std::launch::async, [&, this]()
			{
				std::unique_lock testLock(m_testMtx);
				sem.release();
				bModified = true;
			});

		{
			m_testMtx.unlock();
			using namespace std::chrono_literals;
			status = sem.try_acquire_for(100ms);
		}

		task.get();
		Assert::IsTrue(bModified);
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