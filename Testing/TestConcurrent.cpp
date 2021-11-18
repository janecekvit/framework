#include "stdafx.h"

#include "CppUnitTest.h"
#include "Extensions/Concurrent.h"
#include "Extensions/ResourceWrapper.h"
#include "Extensions/extensions.h"

#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <thread>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FrameworkTesting
{
template <template <class...> class TContainer, class TItem, std::enable_if_t<constraints::is_container_v<TContainer<TItem>>, int> = 0>
void ContainerTest(const TContainer<TItem>& oContainer)
{
	int i = 0;
	for (auto&& oItem : oContainer)
		i++;

	i *= 2;
}

template <template <class...> class TContainer, class TItem, std::enable_if_t<constraints::is_concurrent_container_v<TContainer<TItem>>, int> = 0>
void ContainerTest(const TContainer<TItem>& oContainer)
{
	ContainerTest(oContainer.concurrent().get());
}

TEST_CLASS(TestConcurrent){
	public:
		TEST_METHOD(TestConccurentFunctors){
			int iNumber = 0;
concurrent::functor<void()> fnTest = std::function<void()>([&]()
	{
		iNumber++;
	});

fnTest.exclusive().get()();
fnTest.exclusive().get()();
fnTest.concurrent().get()();

//fnTest.concurrent().begin();

Assert::AreEqual(iNumber, 3);
} // namespace FrameworkTesting
TEST_METHOD(TestConccurentOperations)
{
	std::cout << "Concurent Containers" << std::endl;

	{
		concurrent::resource_owner<std::vector<int>> oVec = std::vector<int>{ 0, 1, 2, 5 };
		oVec.exclusive()->emplace_back(3);
		oVec.exclusive()->emplace_back(4);

		concurrent::resource_owner<std::unordered_map<int, int>> oMap;

		{
			// No [[nodiscard]] attribute
			// oMap.exclusive();
			// oMap.concurrent();

			auto it	   = oMap.exclusive()->begin();
			it		   = oMap.exclusive()->end();
			auto uSize = oMap.exclusive()->size();

			auto itConst = oMap.concurrent()->begin();
			itConst		 = oMap.concurrent()->end();
			uSize		 = oMap.concurrent()->size();
		}

		// exclusive access
		oMap.exclusive()->emplace(5, 5); // exclusive access with lifetime of one operation
		{								 // exclusive access with extended lifetime for more that only one
			auto oScope = oMap.exclusive();
			oScope->emplace(10, 10);
		} // exclusive access ends

		// concurrent access
		auto iResult = oMap.concurrent()->at(5); // concurrent access with lifetime of one operation

		{ // exclusive access with extended lifetime for more that only one
			auto oScope		  = oMap.concurrent();
			auto iResultScope = oScope->at(10);
			auto oScope2	  = oMap.concurrent();
			auto oScope3	  = oMap.concurrent();

		} // concurrent access ends

		{ //exclusive add from scope
			auto oScope = oMap.exclusive();
			oScope->emplace(15, 15);
			oScope->emplace(20, 20);
		}

		{ // concurrent access with scope
			auto oAccess = oMap.concurrent();
			Assert::AreEqual(oMap.concurrent()->at(10), 10);
			auto oAccess2 = oMap.concurrent();

			const int iNumber = extensions::execute_on_container(oMap.concurrent().get(), 20, [&](const int& i)
				{
					return i;
				});

			Assert::AreEqual(iNumber, 20);
		}

		//Test iterators
		{
			std::condition_variable con;
			auto future = std::async(std::launch::async, [&]()
				{
					con.notify_one();
					int i = 5;
					for (auto&& item : oMap.exclusive())
					{
						Assert::AreEqual(item.second, i);
						i += 5;

						item.second += 1;
					}
				});

			std::mutex mutex;
			std::unique_lock<std::mutex> oLock(mutex);
			con.wait(oLock);
			int i = 6;
			for (auto&& item : oMap.exclusive())
			{
				Assert::AreEqual(item.second, i);
				i += 5;
				item.second -= 1;
			}

			future.get();
		}

		//Index operators on unordered map (cannot be const in concurrent access)
		{
			auto oScope = oMap.exclusive();
			oScope[25]	= 25;
		}

		{
			auto oScope = oMap.concurrent();
			Assert::AreEqual(oScope->at(25), 25);
		}

		//Index operators on vector (can access in concurrent, cannot add new item)
		{
			auto oScope = oVec.exclusive();
			oScope[5]	= 5;
		}

		{
			auto oScope = oVec.concurrent();
			Assert::AreEqual(oScope[5], 5);
		}

		//operator ()
		{
			auto oScope = oMap.exclusive();
			oScope.get().emplace(30, 30);
			oScope().emplace(35, 35);

			Assert::AreEqual(oScope[30], 30);
			Assert::AreEqual(oScope[35], 35);
		}

		{
			auto oScope	 = oMap.concurrent();
			auto oScope2 = oMap.concurrent();
			Assert::AreEqual(oScope().at(30), 30);
			Assert::AreEqual(oScope2().at(35), 35);
		}

		//Settter
		std::unordered_map<int, int> oMap2;
		oMap.exclusive().set(std::move(oMap2));

		//Release methods
		try
		{
			auto oScope = oMap.exclusive();
			oScope->emplace(40, 40);
			oScope.release();
			oScope->emplace(45, 45);
		}
		catch (std::system_error&)
		{
			auto oScope = oMap.exclusive();
			Assert::AreEqual(oScope().at(40), 40);
			Assert::AreEqual(oScope().count(45), static_cast<size_t>(0));
		}

		try
		{
			auto oScope = oMap.concurrent();
			Assert::AreEqual(oScope().at(40), 40);
			oScope.release();
			Assert::AreEqual(oScope().at(40), 40);
		}
		catch (std::system_error&)
		{
			auto oScope = oMap.exclusive();
			Assert::AreEqual(oScope().count(40), static_cast<size_t>(1));
		}

		//Move semantics
		auto oOldValue = oMap.exclusive().move();

		std::unordered_map<int, int> oMap3;
		oMap.exclusive().set(std::move(oMap3));

		Assert::AreEqual(oMap.exclusive().get().size(), static_cast<size_t>(0));
		Assert::AreEqual(oOldValue.count(40), static_cast<size_t>(1));

		//Swap semantics
		oOldValue.emplace(50, 50);
		oMap.exclusive()->emplace(100, 100);
		oMap.exclusive().swap(oOldValue);

		Assert::AreEqual(oMap.exclusive().get().size(), static_cast<size_t>(2));
		Assert::AreEqual(oMap.exclusive()->count(50), static_cast<size_t>(1));
		Assert::AreEqual(oMap.exclusive()->count(40), static_cast<size_t>(1));
		Assert::AreEqual(oMap.exclusive()->count(100), static_cast<size_t>(0));

		Assert::AreEqual(oOldValue.size(), static_cast<size_t>(1));
		Assert::AreEqual(oOldValue.count(100), static_cast<size_t>(1));
		Assert::AreEqual(oOldValue.count(50), static_cast<size_t>(0));
		Assert::AreEqual(oOldValue.count(40), static_cast<size_t>(0));

		//user-defined conversion
		int iCalls		= 0;
		auto testLambda = [&](std::unordered_map<int, int>& oMap)
		{
			iCalls++;
		};

		auto testLambdaConst = [&](const std::unordered_map<int, int>& oMap)
		{
			iCalls++;
		};

		testLambda(oMap.exclusive());
		testLambdaConst(oMap.exclusive());
		testLambdaConst(oMap.concurrent());

		Assert::AreEqual(iCalls, 3);

		//std::array
		{
			concurrent::resource_owner<std::array<int, 4>> oArray;
			auto uSize = oArray.exclusive().size();
		}

		{ //test condition varaible
			std::condition_variable_any cv;
			size_t uSize = 0;
			auto oFuture = std::async(std::launch::async, [&uSize, &cv, &oMap]()
				{
					oMap.exclusive().wait(cv);
					oMap.exclusive().wait(cv, [&uSize]()
						{
							return ++uSize == 2;
						});

					oMap.concurrent().wait(cv);
					oMap.concurrent().wait(cv, [&uSize]()
						{
							return ++uSize == 4;
						});
				});

			for (; oFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready;)
				cv.notify_one();

			oFuture.get();
			Assert::AreEqual(uSize, static_cast<size_t>(4));
		}

		{ //re-assign
			auto oScope(std::move(oMap.exclusive()));

			oScope.release();
			oScope = oMap.exclusive();
			Assert::AreEqual(oScope().at(40), 40);
			oScope.release();

			auto oScopeRead(std::move(oMap.concurrent()));
			oScopeRead.release();
			oScopeRead = oMap.concurrent();
			Assert::AreEqual(oScopeRead().at(40), 40);
			oScopeRead.release();
		}

		{ //Acquire write
			auto&& oScope = oMap.exclusive();
			Assert::AreEqual(oScope().at(40), 40);
			oScope.release();

			try
			{
				Assert::AreEqual(oScope().at(40), 40);
			}
			catch (std::system_error&)
			{
				oScope.acquire();
				Assert::AreEqual(oScope().at(40), 40);
				oScope->emplace(222, 222);
			}

			Assert::AreEqual(oScope().at(222), 222);

			try
			{
				oScope.acquire();
			}
			catch (std::system_error&)
			{
				oScope->emplace(226, 226);
			}

			Assert::AreEqual(oScope().at(226), 226);
		}

		{ //Acquire read
			auto&& oScope = oMap.concurrent();
			Assert::AreEqual(oScope().at(40), 40);
			oScope.release();

			try
			{
				Assert::AreEqual(oScope().at(40), 40);
			}
			catch (std::system_error&)
			{
				oScope.acquire();
				Assert::AreEqual(oScope().at(40), 40);
				oScope.release();
				oMap.exclusive()->emplace(223, 223);
			}

			oScope.acquire();
			Assert::AreEqual(oScope().at(223), 223);
		}

		{ //begin const

			// oMap.concurrent()->begin()->second++;
			oMap.exclusive()->begin()->second++;
		}
	}
}

TEST_METHOD(TestConccurentconstraints)
{
	concurrent::list<int> listNumbers = std::list<int>{
		1,
		2,
		3,
		4,
		5
	};

	//ContainerTest(listNumbers);
}
}
;
} // namespace FrameworkTesting
