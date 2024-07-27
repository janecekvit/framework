#include "stdafx.h"

#include "CppUnitTest.h"

#include <future>
#include <iostream>
#include <string>

#define TEST_SIZE 8

#include "Thread\ThreadPool.h"
#include "extensions/constraints.h"
using namespace janecekvit::thread;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FrameworkTesting
{
std::mutex global_lock;
std::atomic<size_t> m_uSyncTasks = 0;
class SyncThreadPoolTest
{
public:
	long m_ltestNumber;
	char m_cChar;
	SyncThreadPoolTest(long lNumber, char ch)
	{
		m_ltestNumber = lNumber;
		m_cChar		  = ch;
	}
	~SyncThreadPoolTest() = default;
	void Summary()
	{
#ifdef VERBOSE
		global_lock.lock();
		std::cout << "Position: " << m_ltestNumber << " " << m_cChar << "1 \n";
		global_lock.unlock();
#endif

		std::this_thread::sleep_for(std::chrono::milliseconds(250));
		m_uSyncTasks++;
	}

	int SummaryTwo(int i)
	{
#ifdef VERBOSE
		global_lock.lock();
		std::cout << "Position: " << m_ltestNumber << " " << m_cChar << "2 \n";
		global_lock.unlock();
#endif

		std::this_thread::sleep_for(std::chrono::milliseconds(250));
		m_uSyncTasks++;
		return i;
	}
};

void SyncThreadStructures()
{
	const size_t uTestSize = 8;
	clock_t begin		   = clock();
	auto pThreadPool	   = std::make_unique<ThreadPool>(uTestSize, [](const std::exception& ex)
		  {
			  std::cerr << ex.what() << std::endl;
		  });

	// A. Using vector to store class
	std::vector<SyncThreadPoolTest> vecOfTests;

	for (unsigned int i = 0; i < uTestSize; i++)
	{
		// 1. AddTask
		// 2. std::bind
		// 3. Function adress
		// 4. Pointer to class (not adress to pointer) // Without &
		// 5. Function _TArgs .....

		// Hint: If you using unique_ptr, you must use std::move for Pointer to Class

		// A. Using vector to store class
		vecOfTests.emplace_back(i, 'A');
		// A1. Bind Summary method without args
		pThreadPool->AddTask([&]()
			{
				auto oItem = vecOfTests.back();
				oItem.Summary();
			});
		// A2. Bind Summary method with one arg
		pThreadPool->AddTask([&]()
			{
				auto oItem = vecOfTests.back();
				oItem.SummaryTwo(2);
			});

		// B. Without class data save
		SyncThreadPoolTest test(i, 'B');
		// B1. Bind Summary method without args
		pThreadPool->AddTask([&test]()
			{
				test.Summary();
			});
		// B2. Bind Summary method with one arg
		pThreadPool->AddTask([&test]()
			{
				test.SummaryTwo(2);
			});
	}
	pThreadPool->WaitAll();
	pThreadPool.reset();
	clock_t end			= clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

	Logger::WriteMessage(std::string("Time: " + std::to_string(elapsed_secs) + "\n").c_str());

	Assert::AreEqual(static_cast<size_t>(32), m_uSyncTasks.load());
}

ONLY_USED_AT_NAMESPACE_SCOPE class TestThread : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<TestThread>
{
public:
	TEST_METHOD(TestSyncThread)
	{
		SyncThreadStructures();
		// system("pause");
	} // namespace FrameworkTesting

	TEST_METHOD(TestSyncThreadDynamic)
	{
		// TODO:
		// SyncThreadStructuresDynamic();
		// system("pause");
	}
};
} // namespace FrameworkTesting
