#include "stdafx.h"

#include "CppUnitTest.h"
#include "Framework/Extensions/Concurrent.h"
#include "Framework/Extensions/Extensions.h"
#include "Framework/Extensions/ResourceWrapper.h"

#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <thread>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FrameworkUT
{
template <template <class...> class TContainer, class TItem, std::enable_if_t<Constraints::is_container_v<TContainer<TItem>>, int> = 0>
void ContainerTest(const TContainer<TItem>& oContainer)
{
	int i = 0;
	for (auto&& oItem : oContainer)
		i++;

	i *= 2;
}

template <template <class...> class TContainer, class TItem, std::enable_if_t<Constraints::is_concurrent_container_v<TContainer<TItem>>, int> = 0>
void ContainerTest(const TContainer<TItem>& oContainer)
{
	ContainerTest(oContainer.Concurrent().Get());
}

TEST_CLASS(TestConcurrent){
	public:
		TEST_METHOD(TestConccurentFunctors){
			int iNumber = 0;
Concurrent::Functor<void()> fnTest = std::function<void()>([&]()
	{
		iNumber++;
	});

fnTest.Exclusive().Get()();
fnTest.Exclusive().Get()();
fnTest.Concurrent().Get()();

//fnTest.Concurrent().begin();

Assert::AreEqual(iNumber, 3);
} // namespace FrameworkUT
TEST_METHOD(TestConccurentOperations)
{
	std::cout << "Concurent Containers" << std::endl;

	{
		Concurrent::ResourceOwner<std::vector<int>> oVec = std::vector<int>{ 0, 1, 2, 5 };
		oVec.Exclusive()->emplace_back(3);
		oVec.Exclusive()->emplace_back(4);

		Concurrent::ResourceOwner<std::unordered_map<int, int>> oMap;

		{
			// No [[nodiscard]] attribute
			// oMap.Exclusive();
			// oMap.Concurrent();

			auto it	   = oMap.Exclusive()->begin();
			it		   = oMap.Exclusive()->end();
			auto uSize = oMap.Exclusive()->size();

			auto itConst = oMap.Concurrent()->begin();
			itConst		 = oMap.Concurrent()->end();
			uSize		 = oMap.Concurrent()->size();
		}

		// Exclusive access
		oMap.Exclusive()->emplace(5, 5); // exclusive access with lifetime of one operation
		{								 // exclusive access with extended lifetime for more that only one
			auto oScope = oMap.Exclusive();
			oScope->emplace(10, 10);
		} // exclusive access ends

		// Concurrent access
		auto iResult = oMap.Concurrent()->at(5); // concurrent access with lifetime of one operation

		{ // exclusive access with extended lifetime for more that only one
			auto oScope		  = oMap.Concurrent();
			auto iResultScope = oScope->at(10);
			auto oScope2	  = oMap.Concurrent();
			auto oScope3	  = oMap.Concurrent();

		} // concurrent access ends

		{ //Exclusive add from scope
			auto oScope = oMap.Exclusive();
			oScope->emplace(15, 15);
			oScope->emplace(20, 20);
		}

		{ // concurrent access with scope
			auto oAccess = oMap.Concurrent();
			Assert::AreEqual(oMap.Concurrent()->at(10), 10);
			auto oAccess2 = oMap.Concurrent();

			const int iNumber = Extensions::ContainerFind(oMap.Concurrent().Get(), 20, [&](const int& i)
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
					for (auto&& item : oMap.Exclusive())
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
			for (auto&& item : oMap.Exclusive())
			{
				Assert::AreEqual(item.second, i);
				i += 5;
				item.second -= 1;
			}

			future.get();
		}

		//Index operators on unordered map (cannot be const in concurrent access)
		{
			auto oScope = oMap.Exclusive();
			oScope[25]	= 25;
		}

		{
			auto oScope = oMap.Concurrent();
			Assert::AreEqual(oScope->at(25), 25);
		}

		//Index operators on vector (can access in concurrent, cannot add new item)
		{
			auto oScope = oVec.Exclusive();
			oScope[5]	= 5;
		}

		{
			auto oScope = oVec.Concurrent();
			Assert::AreEqual(oScope[5], 5);
		}

		//operator ()
		{
			auto oScope = oMap.Exclusive();
			oScope.Get().emplace(30, 30);
			oScope().emplace(35, 35);

			Assert::AreEqual(oScope[30], 30);
			Assert::AreEqual(oScope[35], 35);
		}

		{
			auto oScope	 = oMap.Concurrent();
			auto oScope2 = oMap.Concurrent();
			Assert::AreEqual(oScope().at(30), 30);
			Assert::AreEqual(oScope2().at(35), 35);
		}

		//Settter
		std::unordered_map<int, int> oMap2;
		oMap.Exclusive().Set(std::move(oMap2));

		//Release methods
		try
		{
			auto oScope = oMap.Exclusive();
			oScope->emplace(40, 40);
			oScope.Release();
			oScope->emplace(45, 45);
		}
		catch (std::system_error&)
		{
			auto oScope = oMap.Exclusive();
			Assert::AreEqual(oScope().at(40), 40);
			Assert::AreEqual(oScope().count(45), static_cast<size_t>(0));
		}

		try
		{
			auto oScope = oMap.Concurrent();
			Assert::AreEqual(oScope().at(40), 40);
			oScope.Release();
			Assert::AreEqual(oScope().at(40), 40);
		}
		catch (std::system_error&)
		{
			auto oScope = oMap.Exclusive();
			Assert::AreEqual(oScope().count(40), static_cast<size_t>(1));
		}

		//Move semantics
		auto oOldValue = oMap.Exclusive().Move();

		std::unordered_map<int, int> oMap3;
		oMap.Exclusive().Set(std::move(oMap3));

		Assert::AreEqual(oMap.Exclusive().Get().size(), static_cast<size_t>(0));
		Assert::AreEqual(oOldValue.count(40), static_cast<size_t>(1));

		//Swap semantics
		oOldValue.emplace(50, 50);
		oMap.Exclusive()->emplace(100, 100);
		oMap.Exclusive().Swap(oOldValue);

		Assert::AreEqual(oMap.Exclusive().Get().size(), static_cast<size_t>(2));
		Assert::AreEqual(oMap.Exclusive()->count(50), static_cast<size_t>(1));
		Assert::AreEqual(oMap.Exclusive()->count(40), static_cast<size_t>(1));
		Assert::AreEqual(oMap.Exclusive()->count(100), static_cast<size_t>(0));

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

		testLambda(oMap.Exclusive());
		testLambdaConst(oMap.Exclusive());
		testLambdaConst(oMap.Concurrent());

		Assert::AreEqual(iCalls, 3);

		//std::array
		{
			Concurrent::ResourceOwner<std::array<int, 4>> oArray;
			auto uSize = oArray.Exclusive().size();
		}

		{ //test condition varaible
			std::condition_variable_any cv;
			size_t uSize = 0;
			auto oFuture = std::async(std::launch::async, [&uSize, &cv, &oMap]()
				{
					oMap.Exclusive().Wait(cv);
					oMap.Exclusive().Wait(cv, [&uSize]()
						{
							return ++uSize == 2;
						});

					oMap.Concurrent().Wait(cv);
					oMap.Concurrent().Wait(cv, [&uSize]()
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
			auto oScope(std::move(oMap.Exclusive()));

			oScope.Release();
			oScope = oMap.Exclusive();
			Assert::AreEqual(oScope().at(40), 40);
			oScope.Release();

			auto oScopeRead(std::move(oMap.Concurrent()));
			oScopeRead.Release();
			oScopeRead = oMap.Concurrent();
			Assert::AreEqual(oScopeRead().at(40), 40);
			oScopeRead.Release();
		}

		{ //Acquire write
			auto&& oScope = oMap.Exclusive();
			Assert::AreEqual(oScope().at(40), 40);
			oScope.Release();

			try
			{
				Assert::AreEqual(oScope().at(40), 40);
			}
			catch (std::system_error&)
			{
				oScope.Acquire();
				Assert::AreEqual(oScope().at(40), 40);
				oScope->emplace(222, 222);
			}

			Assert::AreEqual(oScope().at(222), 222);

			try
			{
				oScope.Acquire();
			}
			catch (std::system_error&)
			{
				oScope->emplace(226, 226);
			}

			Assert::AreEqual(oScope().at(226), 226);
		}

		{ //Acquire read
			auto&& oScope = oMap.Concurrent();
			Assert::AreEqual(oScope().at(40), 40);
			oScope.Release();

			try
			{
				Assert::AreEqual(oScope().at(40), 40);
			}
			catch (std::system_error&)
			{
				oScope.Acquire();
				Assert::AreEqual(oScope().at(40), 40);
				oScope.Release();
				oMap.Exclusive()->emplace(223, 223);
			}

			oScope.Acquire();
			Assert::AreEqual(oScope().at(223), 223);
		}

		{ //begin const

			// oMap.Concurrent()->begin()->second++;
			oMap.Exclusive()->begin()->second++;
		}
	}
}

TEST_METHOD(TestConccurentConstraints)
{
	Concurrent::List<int> listNumbers = std::list<int>{
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
} // namespace FrameworkUT
