#include "extensions/extensions.h"
#include "storage/resource_wrapper.h"
#include "synchronization/concurrent.h"

#include <fstream>
#include <future>
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <thread>

using namespace janecekvit;
using namespace janecekvit::synchronization;

namespace framework_tests
{

constexpr size_t IterationCount = 500000;

class test_concurrent : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}

	[[nodiscard]] static concurrent::map<int, int> _prepare_testing_data()
	{
		concurrent::map<int, int> container;
		container.exclusive()->emplace(5, 5); // exclusive access with lifetime of one operation

		// exclusive access with extended lifetime for more that only one
		auto scope = container.exclusive();
		scope->emplace(10, 10);
		scope->emplace(15, 15);

		return container;
	}

	[[nodiscard]] static concurrent::resource_owner<int> _prepare_testing_data_perf_test()
	{
		concurrent::resource_owner<int> container;
		container.exclusive()--; // exclusive access with lifetime of one operation

		// exclusive access with extended lifetime for more that only one
		auto scope = container.exclusive();
		scope++;
		scope++;

		return container;
	}
};

TEST_F(test_concurrent, TestContainerAliases)
{
	auto uset = concurrent::unordered_set<int>();
	auto umultiset = concurrent::unordered_multiset<int>();
	auto umap = concurrent::unordered_map<int, int>();
	auto umultimap = concurrent::unordered_multimap<int, int>();
	auto set = concurrent::set<int>();
	auto multiset = concurrent::multiset<int>();
	auto map = concurrent::map<int, int>();
	auto multimap = concurrent::multimap<int, int>();
	auto list = concurrent::list<int>();
	auto vector = concurrent::vector<int>();
	auto stack = concurrent::stack<int>();
	auto queue = concurrent::queue<int>();
	// auto array	   = concurrent::array<int, 4>();
	auto functor = concurrent::functor<int()>();

	concurrent::resource_owner<std::array<int, 4>> oArray;
	auto size = oArray.exclusive().size();
	ASSERT_EQ(size, 4);
}

TEST_F(test_concurrent, TestExclusiveAccessDirect)
{
	concurrent::unordered_map<int, int> container;
	container.exclusive()->emplace(5, 5); // exclusive access with lifetime of one operation
	ASSERT_EQ(container.exclusive()->at(5), 5);
}

TEST_F(test_concurrent, TestExclusiveAccessScope)
{
	auto container = _prepare_testing_data();

	auto scope = container.exclusive();
	ASSERT_EQ(scope->at(5), 5);
	ASSERT_EQ(scope->at(10), 10);
	ASSERT_EQ(scope->at(15), 15);
	ASSERT_THROW(
		{
			std::ignore = scope->at(20);
		},
		std::out_of_range);
}

TEST_F(test_concurrent, TestExclusiveAccessMultipleThreads)
{
	auto&& container = _prepare_testing_data_perf_test();

	ASSERT_EQ((int) container.exclusive(), 1);
	auto thread1 = std::jthread([&]()
		{
			for (size_t i = 0; i < IterationCount; i++)
			{
				auto&& writer = container.exclusive();
				writer++;
			}
		});

	auto thread2 = std::jthread([&]()
		{
			for (size_t i = 0; i < IterationCount; i++)
			{
				auto&& writer = container.exclusive();
				writer++;
			}
		});

	thread1.join();
	thread2.join();
	ASSERT_EQ((int) container.exclusive(), (int) IterationCount * 2 + 1);
}

TEST_F(test_concurrent, TestConcurrentAccessDirect)
{
	auto container = _prepare_testing_data();

	ASSERT_EQ(container.concurrent()->at(5), 5); // concurrent access with lifetime of one operation
	ASSERT_EQ(container.concurrent()->at(10), 10);
}

TEST_F(test_concurrent, TestConcurrentAccessScope)
{
	auto container = _prepare_testing_data();
	{ // concurrent access with extended lifetime for more operations that can be done simultaneously
		auto scope = container.concurrent();
		auto scope2 = container.concurrent();
		auto scope3 = container.concurrent();

		ASSERT_EQ(scope->at(5), 5);
		ASSERT_EQ(scope->at(10), 10);
		ASSERT_EQ(scope->at(15), 15);
		ASSERT_THROW(
			{
				std::ignore = scope->at(20);
			},
			std::out_of_range);

		ASSERT_EQ(scope2->at(5), 5);
		ASSERT_EQ(scope2->at(10), 10);
		ASSERT_EQ(scope2->at(15), 15);
		ASSERT_THROW(
			{
				std::ignore = scope2->at(20);
			},
			std::out_of_range);
		ASSERT_EQ(scope3->at(5), 5);
		ASSERT_EQ(scope3->at(10), 10);
		ASSERT_EQ(scope3->at(15), 15);
		ASSERT_THROW(
			{
				std::ignore = scope3->at(20);
			},
			std::out_of_range);

		const int result = extensions::execute_on_container(container.concurrent().get(), 10, [&](const int& number)
			{
				return number;
			});

		ASSERT_EQ(result, 10);

	} // concurrent access ends, release all scopes

	{ // concurrent access with scope
		auto oAccess = container.concurrent();
		ASSERT_EQ(container.concurrent()->at(10), 10);
		auto oAccess2 = container.concurrent();
	}
}

TEST_F(test_concurrent, TestConcurrentAccessMultipleThreads)
{
	auto&& container = _prepare_testing_data_perf_test();
	ASSERT_EQ((int) container.concurrent(), 1);
	auto thread1 = std::jthread([&]()
		{
			for (size_t i = 0; i < IterationCount; i++)
			{
				auto&& scope = container.concurrent();
				[[maybe_unused]] int val = scope;
			}
		});

	auto thread2 = std::jthread([&]()
		{
			for (size_t i = 0; i < IterationCount; i++)
			{
				auto&& scope = container.concurrent();
				[[maybe_unused]] int val = scope;
			}
		});

	thread1.join();
	thread2.join();
	ASSERT_EQ((int) container.concurrent(), 1);
}

TEST_F(test_concurrent, TestIterators)
{
	// No [[nodiscard]] attribute
	// container.exclusive();
	// container.concurrent();

	// Test iterators
	auto container = _prepare_testing_data();
	auto begin = container.exclusive()->begin();
	auto end = container.exclusive()->end();
	auto size = container.exclusive()->size();

	ASSERT_EQ(size, 3);
	ASSERT_FALSE(begin == end);
	container.exclusive()->emplace(20, 20);

	auto beginConst = container.concurrent()->begin();
	auto endConst = container.concurrent()->end();
	size = container.concurrent()->size();
	ASSERT_FALSE(beginConst == endConst);
	ASSERT_EQ(size, 4);

	{ // begin const

		auto beginScope = container.exclusive()->begin();

		// beginConst->second++; //concurrent
		beginScope->second++; // exclusive
		ASSERT_EQ(beginScope->second, 6);
	}
}

TEST_F(test_concurrent, TestRangeLoop)
{
	int result = 0;
	auto container = _prepare_testing_data();

	for (auto&& item : container.exclusive())
	{
		result += 5;
		ASSERT_EQ(item.first, result);
	}

	result = 0;
	for (auto&& item : container.concurrent())
	{
		result += 5;
		ASSERT_EQ(item.first, result);
	}
}

TEST_F(test_concurrent, TestExclusiveAccessSynchroAsync)
{
	auto container = _prepare_testing_data();

	std::promise<void> promise;
	auto future = promise.get_future();

	auto async_task = std::async(std::launch::async, [&]()
		{
			int i = 5;
			for (auto&& item : container.exclusive())
			{
				ASSERT_EQ(item.second, i);
				i += 5;
				item.second += 1;
			}
			promise.set_value();
		});

	// Wait for first thread to complete
	future.wait();

	int i = 6;
	for (auto&& item : container.exclusive())
	{
		ASSERT_EQ(item.second, i);
		i += 5;
		item.second -= 1;
	}

	async_task.get();
}

TEST_F(test_concurrent, TestIndexOperator)
{
	auto container = concurrent::vector<int>();

	// Index operators on unordered map
	{
		auto scope = container.exclusive();
		ASSERT_THROW(
			{
				[[maybe_unused]] auto result = scope->at(20);
			},
			std::out_of_range);

		scope->resize(5);
		scope[0] = 25;
	}

	{
		auto scope = container.concurrent();
		ASSERT_EQ(scope[0], 25);
	}
}

TEST_F(test_concurrent, TestFunctionCallOperator)
{
	auto container = _prepare_testing_data();
	{
		auto scope = container.exclusive();
		scope().emplace(30, 30);
		scope().emplace(35, 35);

		ASSERT_EQ(scope[30], 30);
		ASSERT_EQ(scope[35], 35);
	}

	{
		auto scope = container.concurrent();
		auto scope2 = container.concurrent();
		ASSERT_EQ(scope().at(30), 30);
		ASSERT_EQ(scope2().at(35), 35);
	}
}

TEST_F(test_concurrent, TestSwapSemantics)
{
	auto container = _prepare_testing_data();

	std::map<int, int> tmpContainer;
	tmpContainer.emplace(30, 30);
	tmpContainer.emplace(35, 35);

	auto scope = container.exclusive();
	scope.swap(tmpContainer);

	ASSERT_THROW(
		{
			[[maybe_unused]] auto result = scope->at(5);
		},
		std::out_of_range);
	ASSERT_THROW(
		{
			[[maybe_unused]] auto result = scope->at(10);
		},
		std::out_of_range);
	ASSERT_THROW(
		{
			[[maybe_unused]] auto result = scope->at(15);
		},
		std::out_of_range);
	ASSERT_EQ(scope[30], 30);
	ASSERT_EQ(scope[35], 35);

	ASSERT_EQ(tmpContainer[5], 5);
	ASSERT_EQ(tmpContainer[10], 10);
	ASSERT_EQ(tmpContainer[15], 15);
	ASSERT_THROW(
		{
			[[maybe_unused]] auto result = tmpContainer.at(30);
		},
		std::out_of_range);
	ASSERT_THROW(
		{
			[[maybe_unused]] auto result = tmpContainer.at(35);
		},
		std::out_of_range);
}

TEST_F(test_concurrent, TestMoveSemantics)
{
	auto container = _prepare_testing_data();
	auto&& scope = container.exclusive();
	auto tmpContainer = scope.move();

	ASSERT_EQ(scope.size(), 0);
	ASSERT_EQ(tmpContainer[5], 5);
	ASSERT_EQ(tmpContainer[10], 10);
	ASSERT_EQ(tmpContainer[15], 15);
}

TEST_F(test_concurrent, TestSetSemantics)
{
	auto container = _prepare_testing_data();

	std::map<int, int> tmpContainer;
	tmpContainer.emplace(30, 30);
	tmpContainer.emplace(35, 35);

	auto scope = container.exclusive();
	scope.set(std::move(tmpContainer));

	ASSERT_EQ(scope[30], 30);
	ASSERT_EQ(scope[35], 35);

	ASSERT_THROW(
		{
			[[maybe_unused]] auto result = scope->at(5);
		},
		std::out_of_range);
	ASSERT_THROW(
		{
			[[maybe_unused]] auto result = scope->at(10);
		},
		std::out_of_range);
	ASSERT_THROW(
		{
			[[maybe_unused]] auto result = scope->at(15);
		},
		std::out_of_range);

	ASSERT_EQ(tmpContainer.size(), 0);
}

TEST_F(test_concurrent, TestGetSemantics)
{
	auto container = _prepare_testing_data();
	{
		auto scope = container.exclusive();
		scope.get().emplace(30, 30);
		scope.get().emplace(35, 35);

		ASSERT_EQ(scope[30], 30);
		ASSERT_EQ(scope[35], 35);
	}

	{
		auto scope = container.concurrent();
		auto scope2 = container.concurrent();
		ASSERT_EQ(scope.get().at(30), 30);
		ASSERT_EQ(scope2.get().at(35), 35);
	}
}

TEST_F(test_concurrent, TestUserDefinedConversion)
{
	int iCalls = 0;
	auto container = _prepare_testing_data();
	auto testLambda = [&](std::map<int, int>&)
	{
		iCalls++;
	};

	auto testLambdaConst = [&](const std::map<int, int>&)
	{
		iCalls++;
	};

	testLambda(container.exclusive());
	testLambdaConst(container.exclusive());
	// testLambda(container.concurrent()); -> cannot be done due constless
	testLambdaConst(container.concurrent());

	ASSERT_EQ(iCalls, 3);
}

TEST_F(test_concurrent, TestSynchronisationSemantics)
{
	size_t size = 0;
	std::condition_variable_any cv;
	auto container = _prepare_testing_data();
	auto oFuture = std::async(std::launch::async, [&size, &cv, &container]()
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
	ASSERT_EQ(size, 4);
}

TEST_F(test_concurrent, TestUnlockSemantics)
{
	auto container = _prepare_testing_data();
	{
		auto scope = container.exclusive();
		ASSERT_TRUE(scope().contains(15));
		scope.unlock();

		ASSERT_THROW(
			{
				[[maybe_unused]] auto result = scope->at(15);
			},
			std::system_error);
	}

	auto scope = container.concurrent();
	ASSERT_EQ(scope().at(15), 15);
	scope.unlock();
	ASSERT_THROW(
		{
			[[maybe_unused]] auto result = scope->at(15);
		},
		std::system_error);
	ASSERT_THROW(
		{
			scope.unlock();
		},
		std::system_error);
}

TEST_F(test_concurrent, TestLockSemantics)
{
	auto container = _prepare_testing_data();
	{ // lock write
		auto&& scope = container.exclusive();
		scope->emplace(40, 40);
		ASSERT_EQ(scope().at(40), 40);
		scope.unlock();
		ASSERT_THROW(
			{
				[[maybe_unused]] auto result = scope->at(40);
			},
			std::system_error);
		scope.lock();
		ASSERT_EQ(scope().at(40), 40);
		scope->emplace(222, 222);
		ASSERT_EQ(scope().at(222), 222);

		ASSERT_THROW(
			{
				scope.lock();
			},
			std::system_error);

		scope->emplace(226, 226);
		ASSERT_EQ(scope().at(226), 226);
	}

	{ // Acquire read
		auto&& scope = container.concurrent();
		ASSERT_EQ(scope().at(40), 40);
		scope.unlock();

		ASSERT_THROW(
			{
				std::ignore = scope->at(40);
			},
			std::system_error);
		scope.lock();
		ASSERT_EQ(scope().at(40), 40);
		scope.unlock();
		container.exclusive()->emplace(223, 223);

		scope.lock();
		ASSERT_EQ(scope().at(223), 223);
	}
}

TEST_F(test_concurrent, TestReassign)
{
	auto container = _prepare_testing_data();
	// exclusive
	auto scope(container.exclusive());
	ASSERT_THROW(
		{
			scope.lock();
		},
		std::system_error);

	ASSERT_EQ(scope().at(15), 15);
	scope.unlock();
	ASSERT_THROW(
		{
			scope.unlock();
		},
		std::system_error);

	scope = container.exclusive();
	ASSERT_THROW(
		{
			scope.lock();
		},
		std::system_error);

	ASSERT_EQ(scope().at(15), 15);
	scope.unlock();
	ASSERT_THROW(
		{
			scope.unlock();
		},
		std::system_error);

	// concurrent
	auto scopeRead(container.concurrent());
	ASSERT_THROW(
		{
			scopeRead.lock();
		},
		std::system_error);
	ASSERT_EQ(scopeRead().at(15), 15);
	scopeRead.unlock();
	ASSERT_THROW(
		{
			scopeRead.unlock();
		},
		std::system_error);

	scopeRead = container.concurrent();
	ASSERT_THROW(
		{
			scopeRead.lock();
		},
		std::system_error);
	ASSERT_EQ(scopeRead().at(15), 15);
	scopeRead.unlock();
	ASSERT_THROW(
		{
			scopeRead.unlock();
		},
		std::system_error);
}

TEST_F(test_concurrent, TestContainerCopy)
{
	{ // exclusive
		concurrent::map<int, int> container2;
		int originalValue = 0;
		{
			auto container = _prepare_testing_data();
			auto&& scope = container.exclusive();
			ASSERT_EQ(scope->at(15), 15);
			originalValue = scope->at(15);
			scope.unlock();
			container2 = container;
		}

		auto scope = container2.exclusive();
		ASSERT_EQ(scope->at(15), originalValue);

		scope.unlock();
		scope.lock();

		ASSERT_EQ(scope->at(15), originalValue);
	}

	{ // concurrent
		concurrent::map<int, int> container2;
		int originalValue = 0;

		{
			auto container = _prepare_testing_data();
			auto scope = container.concurrent();
			ASSERT_EQ(scope->at(15), 15);
			originalValue = scope->at(15);
			scope.unlock();
			container2 = container;
		}

		auto scope = container2.concurrent();
		ASSERT_EQ(scope->at(15), originalValue);

		scope.unlock();
		scope.lock();

		ASSERT_EQ(scope->at(15), originalValue);
	}
}

TEST_F(test_concurrent, TestDebugLocksInformation)
{
	concurrent::resource_owner_debug<std::unordered_set<int>> container;
	{ // exclusive
		ASSERT_FALSE(container.get_exclusive_lock_details().has_value());
		{
			[[maybe_unused]] auto&& oScope = container.exclusive();
			ASSERT_TRUE(container.get_exclusive_lock_details().has_value());
		}

		ASSERT_FALSE(container.get_exclusive_lock_details().has_value());
		auto&& oScope = container.exclusive();
		ASSERT_TRUE(container.get_exclusive_lock_details().has_value());
		oScope.unlock();
		ASSERT_FALSE(container.get_exclusive_lock_details().has_value());
		oScope.lock();
		ASSERT_TRUE(container.get_exclusive_lock_details().has_value());
	}

	{ // concurrent
		ASSERT_TRUE(container.get_concurrent_lock_details().empty());
		{
			[[maybe_unused]] auto&& oScope = container.concurrent();
			[[maybe_unused]] auto&& oScope2 = container.concurrent();

			ASSERT_EQ((size_t) 2, container.get_concurrent_lock_details().size());
		}

		ASSERT_TRUE(container.get_concurrent_lock_details().empty());
		auto&& oScope = container.concurrent();
		ASSERT_FALSE(container.get_concurrent_lock_details().empty());
		oScope.unlock();
		ASSERT_TRUE(container.get_concurrent_lock_details().empty());
		oScope.lock();
		ASSERT_FALSE(container.get_concurrent_lock_details().empty());
	}
}
} // namespace framework_tests
