#include "stdafx.h"

#include "CppUnitTest.h"

#include <future>
#include <iostream>
#include <string>

#define TEST_SIZE 8

#include "Thread\ThreadPool.h"
#include "Thread\ThreadPoolDynamic.h"
#include "extensions/constraints.h"

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

	//A. Using vector to store class
	std::vector<SyncThreadPoolTest> vecOfTests;

	for (unsigned int i = 0; i < uTestSize; i++)
	{
		//1. AddTask
		//2. std::bind
		//3. Function adress
		//4. Pointer to class (not adress to pointer) // Without &
		//5. Function _TArgs .....

		//Hint: If you using unique_ptr, you must use std::move for Pointer to Class

		//A. Using vector to store class
		vecOfTests.emplace_back(i, 'A');
		//A1. Bind Summary method without args
		pThreadPool->AddTask([&]()
			{
				auto oItem = vecOfTests.back();
				oItem.Summary();
			});
		//A2. Bind Summary method with one arg
		pThreadPool->AddTask([&]()
			{
				auto oItem = vecOfTests.back();
				oItem.SummaryTwo(2);
			});

		//B. Without class data save
		SyncThreadPoolTest test(i, 'B');
		//B1. Bind Summary method without args
		pThreadPool->AddTask([&test]()
			{
				test.Summary();
			});
		//B2. Bind Summary method with one arg
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

void SyncThreadStructuresDynamic()
{
	m_uSyncTasks	 = 0;
	clock_t begin	 = clock();
	auto pThreadPool = std::make_unique<ThreadPoolDynamic>(8, 0.9, [](const std::exception& ex)
		{
			std::cerr << ex.what() << std::endl;
		});
	//A. Using vector to store class
	std::vector<SyncThreadPoolTest> vecOfTests;
	size_t uCounter	   = 0;
	double iMultiplier = 1.0;
	for (unsigned int i = 0; i < TEST_SIZE + TEST_SIZE; i++)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(i * 10 + 100));
		std::cout << "Actual " << i << ". Pool size.: " << pThreadPool->PoolSize() << "\n";
		Logger::WriteMessage(std::string("Actual " + std::to_string(i) + ". Pool size.: " + std::to_string(pThreadPool->PoolSize()) + "\n").c_str());

		//1. AddTask
		//2. std::bind
		//3. Function adress
		//4. Pointer to class (not adress to pointer) // Without &
		//5. Function _TArgs .....

		//Hint: If you using unique_ptr, you must use std::move for Pointer to Class
		if (i < TEST_SIZE / 2)
		{
			iMultiplier += 0.8;
		}
		else if (i > TEST_SIZE / 2)
		{
			iMultiplier -= 0.8;
		}
		for (unsigned int j = 0; j < TEST_SIZE * iMultiplier; j++)
		{
			uCounter += 4;
			//A. Using vector to store class
			vecOfTests.emplace_back(i + j, 'A');
			//A1. Bind Summary method without args
			pThreadPool->AddTask(std::bind(&SyncThreadPoolTest::Summary, vecOfTests.back()));
			//A2. Bind Summary method with one arg
			pThreadPool->AddTask(std::bind(&SyncThreadPoolTest::SummaryTwo, vecOfTests.back(), 1));

			//B. Without class data save
			SyncThreadPoolTest test(i + j, 'B');
			//B1. Bind Summary method without args
			pThreadPool->AddTask(std::bind(&SyncThreadPoolTest::Summary, test));
			//B2. Bind Summary method with one arg
			pThreadPool->AddTask(std::bind(&SyncThreadPoolTest::SummaryTwo, test, 1));
		}
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	Logger::WriteMessage(std::string("Actual Pool size.: " + std::to_string(pThreadPool->PoolSize()) + "\n").c_str());
	Logger::WriteMessage(std::string("Summary Task count was: " + std::to_string(uCounter) + "\n").c_str());

	clock_t end			= clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

	Logger::WriteMessage(std::string("Time: " + std::to_string(elapsed_secs) + "\n").c_str());

	begin = clock();

	pThreadPool->WaitAll();
	pThreadPool.reset();
	end			 = clock();
	elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	Logger::WriteMessage(std::string("Release resources time: " + std::to_string(elapsed_secs) + "\n").c_str());
	Assert::AreEqual(static_cast<size_t>(828), m_uSyncTasks.load());
}

ONLY_USED_AT_NAMESPACE_SCOPE class TestThread : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<TestThread>
{
public:
	TEST_METHOD(TestSyncThread)
	{
		SyncThreadStructures();
		//system("pause");
	} // namespace FrameworkTesting

	TEST_METHOD(TestSyncThreadDynamic)
	{
		//TODO:
		//SyncThreadStructuresDynamic();
		//system("pause");
	}

	TEST_METHOD(TestConditionVariable)
	{
		std::condition_variable_any conv;
		auto oAsync = std::async(std::launch::async, [&conv]()
			{
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(200ms);
				conv.notify_one();
				std::this_thread::sleep_for(200ms);
				conv.notify_one();
				std::this_thread::sleep_for(200ms);
				conv.notify_one();
			});

		//Create Unique Lock which owns Mutex object and lock it until wait or block end
		std::mutex mtx;
		std::unique_lock<std::mutex> mtxQueueLock(mtx);
		size_t uTest = 0;
		conv.wait(mtxQueueLock, [&uTest]()
			{
				return ++uTest == 3;
			});

		oAsync.get();
	}
};
} // namespace FrameworkTesting
