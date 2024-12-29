#include "stdafx.h"

#include "extensions/extensions.h"
#include "storage/resource_wrapper.h"
#include "synchronization/atomic_concurrent.h"

#include <exception>
#include <fstream>
#include <future>
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <thread>

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

constexpr size_t IterationCount = 500000;

class test_atomic_atomic_concurrent : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}

	[[nodiscard]] static std::unique_ptr<atomic_concurrent::unordered_map<int, int>> _prepare_testing_data()
	{
		auto container = std::make_unique<atomic_concurrent::unordered_map<int, int>>();
		container->writer()->emplace(5, 5); // Writer access with lifetime of one operation

		// Writer access with extended lifetime for more that only one
		auto scope = container->writer();
		scope->emplace(10, 10);
		scope->emplace(15, 15);

		return container;
	}

	[[nodiscard]] static std::unique_ptr<atomic_concurrent::resource_owner<int>> _prepare_testing_data_perf_test()
	{
		auto container = std::make_unique<atomic_concurrent::resource_owner<int>>();
		container->writer()--; // writer access with lifetime of one operation

		// writer access with extended lifetime for more that only one
		auto scope = container->writer();
		scope++;
		scope++;

		return container;
	}

	template <class _Func>
	void _catch_all_exceptions_thow_std_exception(_Func&& callback)
	{
		try
		{
			callback();
		}
		catch (...)
		{
			throw std::exception("access violation");
		}
	}
};

TEST_F(test_atomic_atomic_concurrent, TestContainerAliases)
{
	auto uset = atomic_concurrent::unordered_set<int>();
	auto umultiset = atomic_concurrent::unordered_multiset<int>();
	auto umap = atomic_concurrent::unordered_map<int, int>();
	auto umultimap = atomic_concurrent::unordered_multimap<int, int>();
	auto set = atomic_concurrent::set<int>();
	auto multiset = atomic_concurrent::multiset<int>();
	auto map = atomic_concurrent::map<int, int>();
	auto multimap = atomic_concurrent::multimap<int, int>();
	auto list = atomic_concurrent::list<int>();
	auto vector = atomic_concurrent::vector<int>();
	auto stack = atomic_concurrent::stack<int>();
	auto queue = atomic_concurrent::queue<int>();
	// auto array	   = atomic_concurrent::array<int, 4>();
	auto functor = atomic_concurrent::functor<int()>();

	atomic_concurrent::resource_owner<std::array<int, 4>> oArray;
	auto size = oArray.writer().size();
	ASSERT_EQ(size, 4);
}

TEST_F(test_atomic_atomic_concurrent, TestWriterAccessDirect)
{
	atomic_concurrent::unordered_map<int, int> container;
	container.writer()->emplace(5, 5); // Writer access with lifetime of one operation
	ASSERT_EQ(container.writer()->at(5), 5);
}

TEST_F(test_atomic_atomic_concurrent, TestWriterAccessScope)
{
	auto&& container = _prepare_testing_data();

	{
		ASSERT_TRUE(container->is_writer_free());

		auto optScope = container->try_to_acquire_writer();
		ASSERT_TRUE(optScope.has_value());

		ASSERT_FALSE(container->is_writer_free());

		auto scope = std::move(optScope.value());
		scope[5] = 10;
		scope[10] = 15;
		scope[15] = 20;
	}

	auto scope = container->writer();
	// auto scope2 = container->writer(); this create hang

	ASSERT_EQ(scope->at(5), 10);
	ASSERT_EQ(scope->at(10), 15);
	ASSERT_EQ(scope->at(15), 20);
	ASSERT_THROW(
		{
			std::ignore = scope->at(20);
		},
		std::out_of_range);
}

TEST_F(test_atomic_atomic_concurrent, TestWriterAccessMultipleThreads)
{
	auto&& container = _prepare_testing_data_perf_test();
	ASSERT_EQ((int) container->writer(), 1);
	auto thread1 = std::jthread([&](std::stop_token token)
		{
			for (size_t i = 0; i < IterationCount; i++)
			{
				auto&& writer = container->writer();
				writer++;
			}
		});

	auto thread2 = std::jthread([&](std::stop_token token)
		{
			for (size_t i = 0; i < IterationCount; i++)
			{
				auto&& writer = container->writer();
				writer++;
			}
		});

	thread1.join();
	thread2.join();
	ASSERT_EQ((int) IterationCount * 2 + 1, (int) container->writer());
}
TEST_F(test_atomic_atomic_concurrent, TestReaderAccessDirect)
{
	auto&& container = _prepare_testing_data();

	ASSERT_EQ(container->reader()->at(5), 5); // atomic_concurrent access with lifetime of one operation
	ASSERT_EQ(container->reader()->at(10), 10);
}

TEST_F(test_atomic_atomic_concurrent, TestReaderAccessScope)
{
	auto&& container = _prepare_testing_data();
	{ // atomic_concurrent access with extended lifetime for more operations that can be done simultaneously
		auto scope = container->reader();
		auto scope2 = container->reader();
		auto scope3 = container->reader();

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

		const int number = extensions::execute_on_container(container->reader().get(), 10, [&](const int& number)
			{
				return number;
			});

		ASSERT_EQ(number, 10);

	} // Reader access ends, release all scopes

	{ // Reader access with scope
		auto oAccess = container->reader();
		ASSERT_EQ(container->reader()->at(10), 10);
		auto oAccess2 = container->reader();
	}
}

TEST_F(test_atomic_atomic_concurrent, TestReaderAccessMultipleThreads)
{
	auto&& container = _prepare_testing_data_perf_test();
	ASSERT_EQ((int) container->reader(), 1);
	auto thread1 = std::jthread([&](std::stop_token token)
		{
			for (size_t i = 0; i < IterationCount; i++)
			{
				auto&& reader = container->reader();
				int val = reader;
			}
		});

	auto thread2 = std::jthread([&](std::stop_token token)
		{
			auto&& writer = container->reader();
			for (size_t i = 0; i < IterationCount; i++)
			{
				auto&& reader = container->reader();
				int val = reader;
			}
		});

	thread1.join();
	thread2.join();
	ASSERT_EQ((int) container->reader(), (int) +1);
}

TEST_F(test_atomic_atomic_concurrent, TestIterators)
{
	// No [[nodiscard]] attribute
	// container->writer();
	// container->reader();

	// Test iterators
	auto&& container = _prepare_testing_data();
	auto begin = container->writer()->begin();
	auto end = container->writer()->end();
	auto size = container->writer()->size();

	ASSERT_EQ(size, 3);
	ASSERT_FALSE(begin == end);
	container->writer()->emplace(20, 20);

	auto beginConst = container->reader()->begin();
	auto endConst = container->reader()->end();
	size = container->reader()->size();
	ASSERT_FALSE(beginConst == endConst);
	ASSERT_EQ(size, 4);

	{ // begin const
		auto begin = container->writer()->begin();

		// beginConst->second++; //atomic_concurrent
		begin->second++; // Writer
		ASSERT_EQ(begin->second, 6);
	}
}

TEST_F(test_atomic_atomic_concurrent, TestRangeLoop)
{
	int result = 0;
	auto&& container = _prepare_testing_data();

	for (auto&& item : container->writer())
	{
		result += 5;
		ASSERT_EQ(item.first, result);
	}

	result = 0;
	for (auto&& item : container->reader())
	{
		result += 5;
		ASSERT_EQ(item.first, result);
	}
}

TEST_F(test_atomic_atomic_concurrent, TestWriterAccessSynchroAsync)
{
	auto&& container = _prepare_testing_data();

	std::condition_variable con;
	auto future = std::async(std::launch::async, [&]()
		{
			con.notify_one();
			int i = 5;
			for (auto&& item : container->writer())
			{
				ASSERT_EQ(item.second, i);
				i += 5;

				item.second += 1;
			}
		});

	std::mutex mutex;
	std::unique_lock<std::mutex> oLock(mutex);
	con.wait(oLock);
	int i = 6;
	for (auto&& item : container->writer())
	{
		ASSERT_EQ(item.second, i);
		i += 5;
		item.second -= 1;
	}

	future.get();
}

TEST_F(test_atomic_atomic_concurrent, TestIndexOperator)
{
	auto&& container = atomic_concurrent::vector<int>();

	// Index operators on unordered map
	{
		auto scope = container.writer();
		ASSERT_THROW(
			{
				[[maybe_unused]] auto result = scope->at(20);
			},
			std::out_of_range);

		scope->resize(5);
		scope[0] = 25;
	}

	{
		auto scope = container.reader();
		ASSERT_EQ(scope[0], 25);
	}
}

TEST_F(test_atomic_atomic_concurrent, TestFunctionCallOperator)
{
	auto&& container = _prepare_testing_data();
	{
		auto scope = container->writer();
		scope().emplace(30, 30);
		scope().emplace(35, 35);

		ASSERT_EQ(scope[30], 30);
		ASSERT_EQ(scope[35], 35);
	}

	{
		auto scope = container->reader();
		auto scope2 = container->reader();
		ASSERT_EQ(scope().at(30), 30);
		ASSERT_EQ(scope2().at(35), 35);
	}
}

TEST_F(test_atomic_atomic_concurrent, TestSwapSemantics)
{
	auto&& container = _prepare_testing_data();

	std::unordered_map<int, int> tmpContainer;
	tmpContainer.emplace(30, 30);
	tmpContainer.emplace(35, 35);

	auto scope = container->writer();
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

TEST_F(test_atomic_atomic_concurrent, TestMoveSemantics)
{
	auto&& container = _prepare_testing_data();
	auto&& scope = container->writer();
	auto tmpContainer = scope.move();

	ASSERT_EQ(scope.size(), 0);
	ASSERT_EQ(tmpContainer[5], 5);
	ASSERT_EQ(tmpContainer[10], 10);
	ASSERT_EQ(tmpContainer[15], 15);
}

TEST_F(test_atomic_atomic_concurrent, TestSetSemantics)
{
	auto&& container = _prepare_testing_data();

	std::unordered_map<int, int> tmpContainer;
	tmpContainer.emplace(30, 30);
	tmpContainer.emplace(35, 35);

	auto scope = container->writer();
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

TEST_F(test_atomic_atomic_concurrent, TestGetSemantics)
{
	auto&& container = _prepare_testing_data();
	{
		auto scope = container->writer();
		scope.get().emplace(30, 30);
		scope.get().emplace(35, 35);

		ASSERT_EQ(scope[30], 30);
		ASSERT_EQ(scope[35], 35);
	}

	{
		auto scope = container->reader();
		auto scope2 = container->reader();
		ASSERT_EQ(scope.get().at(30), 30);
		ASSERT_EQ(scope2.get().at(35), 35);
	}
}

TEST_F(test_atomic_atomic_concurrent, TestUserDefinedConversion)
{
	int iCalls = 0;
	auto&& container = _prepare_testing_data();
	auto testLambda = [&](std::unordered_map<int, int>& container)
	{
		iCalls++;
	};

	auto testLambdaConst = [&](const std::unordered_map<int, int>& container)
	{
		iCalls++;
	};

	testLambda(container->writer());
	testLambdaConst(container->writer());
	// testLambda(container->reader()); //-> cannot be done due constless
	testLambdaConst(container->reader());

	ASSERT_EQ(iCalls, 3);
}

TEST_F(test_atomic_atomic_concurrent, TestFunctor)
{
	int iNumber = 0;
	atomic_concurrent::functor<void()> fnTest = std::function<void()>([&]()
		{
			iNumber++;
		});

	fnTest.writer().get()();
	fnTest.writer().get()();
	fnTest.reader().get()();

	ASSERT_EQ(3, iNumber);
}

TEST_F(test_atomic_atomic_concurrent, TestConccurentConstraints)
{
	atomic_concurrent::list<int> listNumbers = std::list<int>{
		1,
		2,
		3,
		4,
		5
	};

	// ContainerTest(listNumbers);
}

TEST_F(test_atomic_atomic_concurrent, FechAdd)
{
	constexpr size_t loopCount = 500000;
	auto&& container = std::atomic<size_t>(0);
	auto thread1 = std::jthread([&]()
		{
			for (size_t i = 0; i < loopCount; i++)
				container++;
		});

	auto thread2 = std::jthread([&]()
		{
			for (size_t i = 0; i < loopCount; i++)
				container++;
		});
	thread1.join();
	thread2.join();
	ASSERT_EQ(loopCount * 2, container.load());
}

} // namespace FrameworkTesting
