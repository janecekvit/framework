#include "stdafx.h"

#include "CppUnitTest.h"
#include "extensions/extensions.h"
#include "storage/resource_wrapper.h"
#include "synchronization/concurrent.h"

#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <thread>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace janecekvit;
using namespace janecekvit::synchronization;

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

constexpr size_t IterationCount = 500000;

ONLY_USED_AT_NAMESPACE_SCOPE class test_concurrent : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_concurrent> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
	[[no_discard]] static concurrent::unordered_map<int, int> _prepare_testing_data()
	{
		concurrent::unordered_map<int, int> container;
		container.exclusive()->emplace(5, 5); // exclusive access with lifetime of one operation

		// exclusive access with extended lifetime for more that only one
		auto scope = container.exclusive();
		scope->emplace(10, 10);
		scope->emplace(15, 15);

		return container;
	}

	[[no_discard]] static concurrent::resource_owner<int> _prepare_testing_data_perf_test()
	{
		concurrent::resource_owner<int> container;
		container.exclusive()--; // exclusive access with lifetime of one operation

		// exclusive access with extended lifetime for more that only one
		auto scope = container.exclusive();
		scope++;
		scope++;

		return container;
	}

public:
	TEST_METHOD(TestContainerAliases)
	{
		auto uset	   = concurrent::unordered_set<int>();
		auto umultiset = concurrent::unordered_multiset<int>();
		auto umap	   = concurrent::unordered_map<int, int>();
		auto umultimap = concurrent::unordered_multimap<int, int>();
		auto set	   = concurrent::set<int>();
		auto multiset  = concurrent::multiset<int>();
		auto map	   = concurrent::map<int, int>();
		auto multimap  = concurrent::multimap<int, int>();
		auto list	   = concurrent::list<int>();
		auto vector	   = concurrent::vector<int>();
		auto stack	   = concurrent::stack<int>();
		auto queue	   = concurrent::queue<int>();
		// auto array	   = concurrent::array<int, 4>();
		auto functor = concurrent::functor<int()>();

		concurrent::resource_owner<std::array<int, 4>> oArray;
		auto size = oArray.exclusive().size();
		Assert::AreEqual<int>(size, 4);
	}

	TEST_METHOD(TestExclusiveAccessDirect)
	{
		concurrent::unordered_map<int, int> container;
		container.exclusive()->emplace(5, 5); // exclusive access with lifetime of one operation
		Assert::AreEqual(container.exclusive()->at(5), 5);
	}

	TEST_METHOD(TestExclusiveAccessScope)
	{
		auto container = _prepare_testing_data();

		auto scope = container.exclusive();
		Assert::AreEqual(scope->at(5), 5);
		Assert::AreEqual(scope->at(10), 10);
		Assert::AreEqual(scope->at(15), 15);
		Assert::ExpectException<std::out_of_range>([&]()
			{
				scope->at(20);
			});
	}

	TEST_METHOD(TestExclusiveAccessMultipleThreads)
	{
		auto&& container = _prepare_testing_data_perf_test();

		Assert::AreEqual((int) container.exclusive(), 1);
		auto thread1 = std::jthread([&](std::stop_token token)
			{
				for (size_t i = 0; i < IterationCount; i++)
				{
					auto&& writer = container.exclusive();
					writer++;
				}
			});

		auto thread2 = std::jthread([&](std::stop_token token)
			{
				for (size_t i = 0; i < IterationCount; i++)
				{
					auto&& writer = container.exclusive();
					writer++;
				}
			});

		thread1.join();
		thread2.join();
		Assert::AreEqual((int) container.exclusive(), (int) IterationCount * 2 + 1);
	}

	TEST_METHOD(TestConcurrentAccessDirect)
	{
		auto container = _prepare_testing_data();

		Assert::AreEqual(container.concurrent()->at(5), 5); // concurrent access with lifetime of one operation
		Assert::AreEqual(container.concurrent()->at(10), 10);
	}

	TEST_METHOD(TestConcurrentAccessScope)
	{
		auto container = _prepare_testing_data();
		{ // concurrent access with extended lifetime for more operations that can be done simultaneously
			auto scope	= container.concurrent();
			auto scope2 = container.concurrent();
			auto scope3 = container.concurrent();

			Assert::AreEqual(scope->at(5), 5);
			Assert::AreEqual(scope->at(10), 10);
			Assert::AreEqual(scope->at(15), 15);
			Assert::ExpectException<std::out_of_range>([&]()
				{
					scope->at(20);
				});

			Assert::AreEqual(scope2->at(5), 5);
			Assert::AreEqual(scope2->at(10), 10);
			Assert::AreEqual(scope2->at(15), 15);
			Assert::ExpectException<std::out_of_range>([&]()
				{
					scope2->at(20);
				});
			Assert::AreEqual(scope3->at(5), 5);
			Assert::AreEqual(scope3->at(10), 10);
			Assert::AreEqual(scope3->at(15), 15);
			Assert::ExpectException<std::out_of_range>([&]()
				{
					scope3->at(20);
				});

			const int number = extensions::execute_on_container(container.concurrent().get(), 10, [&](const int& number)
				{
					return number;
				});

			Assert::AreEqual(number, 10);

		} // concurrent access ends, release all scopes

		{ // concurrent access with scope
			auto oAccess = container.concurrent();
			Assert::AreEqual(container.concurrent()->at(10), 10);
			auto oAccess2 = container.concurrent();
		}
	}

	TEST_METHOD(TestConcurrentAccessMultipleThreads)
	{
		auto&& container = _prepare_testing_data_perf_test();
		Assert::AreEqual((int) container.concurrent(), 1);
		auto thread1 = std::jthread([&](std::stop_token token)
			{
				for (size_t i = 0; i < IterationCount; i++)
				{
					auto&& scope			 = container.concurrent();
					[[maybe_unused]] int val = scope;
				}
			});

		auto thread2 = std::jthread([&](std::stop_token token)
			{
				for (size_t i = 0; i < IterationCount; i++)
				{
					auto&& scope			 = container.concurrent();
					[[maybe_unused]] int val = scope;
				}
			});

		thread1.join();
		thread2.join();
		Assert::AreEqual((int) container.concurrent(), 1);
	}

	TEST_METHOD(TestIterators)
	{
		// No [[nodiscard]] attribute
		// container.exclusive();
		// container.concurrent();

		// Test iterators
		auto container = _prepare_testing_data();
		auto begin	   = container.exclusive()->begin();
		auto end	   = container.exclusive()->end();
		auto size	   = container.exclusive()->size();

		Assert::AreEqual<size_t>(size, 3);
		Assert::IsFalse(begin == end);
		container.exclusive()->emplace(20, 20);

		auto beginConst = container.concurrent()->begin();
		auto endConst	= container.concurrent()->end();
		size			= container.concurrent()->size();
		Assert::IsFalse(beginConst == endConst);
		Assert::AreEqual<size_t>(size, 4);

		{ // begin const

			auto begin = container.exclusive()->begin();

			// beginConst->second++; //concurrent
			begin->second++; // exclusive
			Assert::AreEqual<int>(begin->second, 6);
		}
	}

	TEST_METHOD(TestRangeLoop)
	{
		int result	   = 0;
		auto container = _prepare_testing_data();

		for (auto&& item : container.exclusive())
		{
			result += 5;
			Assert::AreEqual(item.first, result);
		}

		result = 0;
		for (auto&& item : container.concurrent())
		{
			result += 5;
			Assert::AreEqual(item.first, result);
		}
	}

	TEST_METHOD(TestExclusiveAccessSynchroAsync)
	{
		auto container = _prepare_testing_data();

		std::condition_variable con;
		auto future = std::async(std::launch::async, [&]()
			{
				int i = 5;
				for (auto&& item : container.exclusive())
				{
					Assert::AreEqual(item.second, i);
					i += 5;

					item.second += 1;
				}
				con.notify_one();
			});

		std::mutex mutex;
		std::unique_lock<std::mutex> oLock(mutex);
		con.wait(oLock);
		int i = 6;
		for (auto&& item : container.exclusive())
		{
			Assert::AreEqual(item.second, i);
			i += 5;
			item.second -= 1;
		}

		future.get();
	}

	TEST_METHOD(TestIndexOperator)
	{
		auto container = concurrent::vector<int>();

		// Index operators on unordered map
		{
			auto scope = container.exclusive();
			Assert::ExpectException<std::out_of_range>([&]()
				{
					[[maybe_unused]] auto result = scope->at(20);
				});

			scope->resize(5);
			scope[0] = 25;
		}

		{
			auto scope = container.concurrent();
			Assert::AreEqual<const int&>(scope[0], 25);
		}
	}

	TEST_METHOD(TestFunctionCallOperator)
	{
		auto container = _prepare_testing_data();
		{
			auto scope = container.exclusive();
			scope().emplace(30, 30);
			scope().emplace(35, 35);

			Assert::AreEqual(scope[30], 30);
			Assert::AreEqual(scope[35], 35);
		}

		{
			auto scope	= container.concurrent();
			auto scope2 = container.concurrent();
			Assert::AreEqual(scope().at(30), 30);
			Assert::AreEqual(scope2().at(35), 35);
		}
	}

	TEST_METHOD(TestSwapSemantics)
	{
		auto container = _prepare_testing_data();

		std::unordered_map<int, int> tmpContainer;
		tmpContainer.emplace(30, 30);
		tmpContainer.emplace(35, 35);

		auto scope = container.exclusive();
		scope.swap(tmpContainer);

		Assert::ExpectException<std::out_of_range>([&]()
			{
				[[maybe_unused]] auto result = scope->at(5);
			});
		Assert::ExpectException<std::out_of_range>([&]()
			{
				[[maybe_unused]] auto result = scope->at(10);
			});
		Assert::ExpectException<std::out_of_range>([&]()
			{
				[[maybe_unused]] auto result = scope->at(15);
			});
		Assert::AreEqual(scope[30], 30);
		Assert::AreEqual(scope[35], 35);

		Assert::AreEqual(tmpContainer[5], 5);
		Assert::AreEqual(tmpContainer[10], 10);
		Assert::AreEqual(tmpContainer[15], 15);
		Assert::ExpectException<std::out_of_range>([&]()
			{
				[[maybe_unused]] auto result = tmpContainer.at(30);
			});
		Assert::ExpectException<std::out_of_range>([&]()
			{
				[[maybe_unused]] auto result = tmpContainer.at(35);
			});
	}

	TEST_METHOD(TestMoveSemantics)
	{
		auto container	  = _prepare_testing_data();
		auto&& scope	  = container.exclusive();
		auto tmpContainer = scope.move();

		Assert::AreEqual<int>(scope.size(), 0);
		Assert::AreEqual<int>(tmpContainer[5], 5);
		Assert::AreEqual<int>(tmpContainer[10], 10);
		Assert::AreEqual<int>(tmpContainer[15], 15);
	}

	TEST_METHOD(TestSetSemantics)
	{
		auto container = _prepare_testing_data();

		std::unordered_map<int, int> tmpContainer;
		tmpContainer.emplace(30, 30);
		tmpContainer.emplace(35, 35);

		auto scope = container.exclusive();
		scope.set(std::move(tmpContainer));

		Assert::AreEqual(scope[30], 30);
		Assert::AreEqual(scope[35], 35);

		Assert::ExpectException<std::out_of_range>([&]()
			{
				[[maybe_unused]] auto result = scope->at(5);
			});
		Assert::ExpectException<std::out_of_range>([&]()
			{
				[[maybe_unused]] auto result = scope->at(10);
			});
		Assert::ExpectException<std::out_of_range>([&]()
			{
				[[maybe_unused]] auto result = scope->at(15);
			});

		Assert::AreEqual<size_t>(tmpContainer.size(), 0);
	}

	TEST_METHOD(TestGetSemantics)
	{
		auto container = _prepare_testing_data();
		{
			auto scope = container.exclusive();
			scope.get().emplace(30, 30);
			scope.get().emplace(35, 35);

			Assert::AreEqual(scope[30], 30);
			Assert::AreEqual(scope[35], 35);
		}

		{
			auto scope	= container.concurrent();
			auto scope2 = container.concurrent();
			Assert::AreEqual(scope.get().at(30), 30);
			Assert::AreEqual(scope2.get().at(35), 35);
		}
	}

	TEST_METHOD(TestUserDefinedConversion)
	{
		int iCalls		= 0;
		auto container	= _prepare_testing_data();
		auto testLambda = [&](std::unordered_map<int, int>& container)
		{
			iCalls++;
		};

		auto testLambdaConst = [&](const std::unordered_map<int, int>& container)
		{
			iCalls++;
		};

		testLambda(container.exclusive());
		testLambdaConst(container.exclusive());
		// testLambda(container.concurrent()); -> cannot be done due constless
		testLambdaConst(container.concurrent());

		Assert::AreEqual(iCalls, 3);
	}

	TEST_METHOD(TestSynchronisationSemantics)
	{
		size_t size			 = 0;
		size_t notifications = 0;
		std::condition_variable_any cv;
		auto container = _prepare_testing_data();
		auto oFuture   = std::async(std::launch::async, [&size, &cv, &container]()
			  {
				  container.exclusive().wait(cv);
				  container.exclusive().wait(cv, [&size]()
					  {
						  return ++size == 2;
					  });

				  container.concurrent().wait(cv);
				  container.concurrent().wait(cv, [&size]()
					  {
						  return ++size == 4;
					  });
			  });

		for (; oFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready;)
			cv.notify_one();

		oFuture.get();
		Assert::AreEqual<size_t>(size, 4);
	}

	TEST_METHOD(TestUnlockSemantics)
	{
		auto container = _prepare_testing_data();
		{
			auto scope = container.exclusive();
			Assert::IsTrue(scope().contains(15));
			scope.unlock();

			Assert::ExpectException<std::system_error>([&]()
				{
					[[maybe_unused]] auto result = scope->at(15);
				});
		}

		auto scope = container.concurrent();
		Assert::AreEqual(scope().at(15), 15);
		scope.unlock();
		Assert::ExpectException<std::system_error>([&]()
			{
				[[maybe_unused]] auto result = scope->at(15);
			});
		Assert::ExpectException<std::system_error>([&]
			{
				scope.unlock();
			});
	}

	TEST_METHOD(TestLockSemantics)
	{
		auto container = _prepare_testing_data();
		{ // lock write
			auto&& scope = container.exclusive();
			scope->emplace(40, 40);
			Assert::AreEqual(scope().at(40), 40);
			scope.unlock();
			Assert::ExpectException<std::system_error>([&]()
				{
					[[maybe_unused]] auto result = scope->at(40);
				});
			scope.lock();
			Assert::AreEqual(scope().at(40), 40);
			scope->emplace(222, 222);
			Assert::AreEqual(scope().at(222), 222);

			Assert::ExpectException<std::system_error>([&]
				{
					scope.lock();
				});

			scope->emplace(226, 226);
			Assert::AreEqual(scope().at(226), 226);
		}

		{ // Acquire read
			auto&& scope = container.concurrent();
			Assert::AreEqual(scope().at(40), 40);
			scope.unlock();

			Assert::ExpectException<std::system_error>([&]()
				{
					scope->at(40);
				});
			scope.lock();
			Assert::AreEqual(scope().at(40), 40);
			scope.unlock();
			container.exclusive()->emplace(223, 223);

			scope.lock();
			Assert::AreEqual(scope().at(223), 223);
		}
	}

	TEST_METHOD(TestReassign)
	{
		auto container = _prepare_testing_data();
		// exclusive
		auto scope(std::move(container.exclusive()));
		Assert::ExpectException<std::system_error>([&]
			{
				scope.lock();
			});

		Assert::AreEqual(scope().at(15), 15);
		scope.unlock();
		Assert::ExpectException<std::system_error>([&]
			{
				scope.unlock();
			});

		scope = container.exclusive();
		Assert::ExpectException<std::system_error>([&]
			{
				scope.lock();
			});

		Assert::AreEqual(scope().at(15), 15);
		scope.unlock();
		Assert::ExpectException<std::system_error>([&]
			{
				scope.unlock();
			});

		// concurrent
		auto scopeRead(std::move(container.concurrent()));
		Assert::ExpectException<std::system_error>([&]
			{
				scopeRead.lock();
			});
		Assert::AreEqual(scopeRead().at(15), 15);
		scopeRead.unlock();
		Assert::ExpectException<std::system_error>([&]
			{
				scopeRead.unlock();
			});

		scopeRead = container.concurrent();
		Assert::ExpectException<std::system_error>([&]
			{
				scopeRead.lock();
			});
		Assert::AreEqual(scopeRead().at(15), 15);
		scopeRead.unlock();
		Assert::ExpectException<std::system_error>([&]
			{
				scopeRead.unlock();
			});
	}

	TEST_METHOD(TestContainerCopy)
	{
		{ // exclusive
			std::optional<concurrent::unordered_map<int, int>::exclusive_holder_type> scope;
			concurrent::unordered_map<int, int> container2;
			Assert::IsFalse(scope.has_value());

			{
				auto container = _prepare_testing_data();
				scope		   = container.exclusive();
				Assert::IsTrue(scope.has_value());
				Assert::AreEqual(scope.value()->at(15), 15);
				scope->unlock();
				container2 = container;
			}
			scope = container2.exclusive();
			Assert::IsTrue(scope.has_value());
			Assert::AreEqual(scope.value()->at(15), 15);

			scope->unlock();
			scope->lock();

			Assert::AreEqual(scope.value()->at(15), 15);
			scope.reset();
		}

		{ // concurrent
			std::optional<concurrent::unordered_map<int, int>::concurrent_holder_type> scope;
			concurrent::unordered_map<int, int> container2;
			Assert::IsFalse(scope.has_value());

			{
				auto container = _prepare_testing_data();
				scope		   = container.concurrent();
				Assert::IsTrue(scope.has_value());
				Assert::AreEqual(scope.value()->at(15), 15);
				scope->unlock();
				container2 = container;
			}

			scope = container2.concurrent();
			Assert::IsTrue(scope.has_value());
			Assert::AreEqual(scope.value()->at(15), 15);

			scope->unlock();
			scope->lock();

			Assert::AreEqual(scope.value()->at(15), 15);
			scope.reset();
		}
	}

	TEST_METHOD(TestDebugLocksInformation)
	{
		concurrent::resource_owner_debug<std::unordered_set<int>> container;
		{ // exclusive
			Assert::IsFalse(container.get_exclusive_lock_details().has_value());
			{
				auto&& oScope = container.exclusive();
				Assert::IsTrue(container.get_exclusive_lock_details().has_value());
			}

			Assert::IsFalse(container.get_exclusive_lock_details().has_value());
			auto&& oScope = container.exclusive();
			Assert::IsTrue(container.get_exclusive_lock_details().has_value());
			oScope.unlock();
			Assert::IsFalse(container.get_exclusive_lock_details().has_value());
			oScope.lock();
			Assert::IsTrue(container.get_exclusive_lock_details().has_value());
		}

		{ // concurrent
			Assert::IsTrue(container.get_concurrent_lock_details().empty());
			{
				auto&& oScope  = container.concurrent();
				auto&& oScope2 = container.concurrent();

				Assert::AreEqual((size_t) 2, container.get_concurrent_lock_details().size());
			}

			Assert::IsTrue(container.get_concurrent_lock_details().empty());
			auto&& oScope = container.concurrent();
			Assert::IsFalse(container.get_concurrent_lock_details().empty());
			oScope.unlock();
			Assert::IsTrue(container.get_concurrent_lock_details().empty());
			oScope.lock();
			Assert::IsFalse(container.get_concurrent_lock_details().empty());
		}
	}

	// TEST_METHOD(TestFunctor)
	//{
	//	int iNumber						   = 0;
	//	concurrent::functor<void()> fnTest = std::function<void()>([&]()
	//		{
	//			iNumber++;
	//		});

	//	fnTest.exclusive().get()();
	//	fnTest.exclusive().get()();
	//	fnTest.concurrent().get()();

	//	Assert::AreEqual(3, iNumber);
	//}

	// TEST_METHOD(TestConccurentConstraints)
	//{
	//	concurrent::list<int> listNumbers = std::list<int>{
	//		1,
	//		2,
	//		3,
	//		4,
	//		5
	//	};

	//	//ContainerTest(listNumbers);
	//}
};
} // namespace FrameworkTesting
