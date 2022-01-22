#include "stdafx.h"

#include "CppUnitTest.h"
#include "Extensions/extensions.h"
#include "extensions/atomic_concurrent.h"
#include "extensions/resource_wrapper.h"

#include <exception>
#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <thread>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace janecekvit;

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

//template <template <class...> class TContainer, class TItem, std::enable_if_t<constraints::is_concurrent_container_v<TContainer<TItem>>, int> = 0>
//void ContainerTest(const TContainer<TItem>& oContainer)
//{
//	ContainerTest(ocontainer->reader().get());
//}

constexpr size_t IterationCount = 500000;

ONLY_USED_AT_NAMESPACE_SCOPE class test_atomic_atomic_concurrent : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_atomic_atomic_concurrent> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
	[[no_discard]] static std::unique_ptr<atomic_concurrent::unordered_map<int, int>> _prepare_testing_data()
	{
		auto container = std::make_unique<atomic_concurrent::unordered_map<int, int>>();
		container->writer()->emplace(5, 5); // Writer access with lifetime of one operation

		// Writer access with extended lifetime for more that only one
		auto scope = container->writer();
		scope->emplace(10, 10);
		scope->emplace(15, 15);

		return container;
	}

	[[no_discard]] static std::unique_ptr<atomic_concurrent::resource_owner<int>> _prepare_testing_data_perf_test()
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

public:
	TEST_METHOD(TestContainerAliases)
	{
		auto uset	   = atomic_concurrent::unordered_set<int>();
		auto umultiset = atomic_concurrent::unordered_multiset<int>();
		auto umap	   = atomic_concurrent::unordered_map<int, int>();
		auto umultimap = atomic_concurrent::unordered_multimap<int, int>();
		auto set	   = atomic_concurrent::set<int>();
		auto multiset  = atomic_concurrent::multiset<int>();
		auto map	   = atomic_concurrent::map<int, int>();
		auto multimap  = atomic_concurrent::multimap<int, int>();
		auto list	   = atomic_concurrent::list<int>();
		auto vector	   = atomic_concurrent::vector<int>();
		auto stack	   = atomic_concurrent::stack<int>();
		auto queue	   = atomic_concurrent::queue<int>();
		//auto array	   = atomic_concurrent::array<int, 4>();
		auto functor = atomic_concurrent::functor<int()>();

		atomic_concurrent::resource_owner<std::array<int, 4>> oArray;
		auto size = oArray.writer().size();
		Assert::AreEqual<int>(size, 4);
	}

	TEST_METHOD(TestWriterAccessDirect)
	{
		atomic_concurrent::unordered_map<int, int> container;
		container.writer()->emplace(5, 5); // Writer access with lifetime of one operation
		Assert::AreEqual(container.writer()->at(5), 5);
	}

	TEST_METHOD(TestWriterAccessScope)
	{
		auto&& container = _prepare_testing_data();

		{
			Assert::IsTrue(container->is_writer_free());

			auto optScope = container->try_to_acquire_writer();
			Assert::IsTrue(optScope.has_value());

			Assert::IsFalse(container->is_writer_free());

			auto scope = std::move(optScope.value());
			scope[5]   = 10;
			scope[10]  = 15;
			scope[15]  = 20;
		}

		auto scope = container->writer();
		//auto scope2 = container->writer(); this create hang

		Assert::AreEqual(scope->at(5), 10);
		Assert::AreEqual(scope->at(10), 15);
		Assert::AreEqual(scope->at(15), 20);
		Assert::ExpectException<std::out_of_range>([&]()
			{
				scope->at(20);
			});
	}

	TEST_METHOD(TestWriterAccessMultipleThreads)
	{
		auto&& container = _prepare_testing_data_perf_test();
		Assert::AreEqual((int) container->writer(), 1);
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
		Assert::AreEqual((int) IterationCount * 2 + 1, (int) container->writer());
	}
	TEST_METHOD(TestReaderAccessDirect)
	{
		auto&& container = _prepare_testing_data();

		Assert::AreEqual(container->reader()->at(5), 5); // atomic_concurrent access with lifetime of one operation
		Assert::AreEqual(container->reader()->at(10), 10);
	}

	TEST_METHOD(TestReaderAccessScope)
	{
		auto&& container = _prepare_testing_data();
		{ // atomic_concurrent access with extended lifetime for more operations that can be done simultaneously
			auto scope	= container->reader();
			auto scope2 = container->reader();
			auto scope3 = container->reader();

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

			const int number = extensions::execute_on_container(container->reader().get(), 10, [&](const int& number)
				{
					return number;
				});

			Assert::AreEqual(number, 10);

		} // Reader access ends, release all scopes

		{ // Reader access with scope
			auto oAccess = container->reader();
			Assert::AreEqual(container->reader()->at(10), 10);
			auto oAccess2 = container->reader();
		}
	}

	TEST_METHOD(TestReaderAccessMultipleThreads)
	{
		auto&& container = _prepare_testing_data_perf_test();
		Assert::AreEqual((int) container->reader(), 1);
		auto thread1 = std::jthread([&](std::stop_token token)
			{
				for (size_t i = 0; i < IterationCount; i++)
				{
					auto&& reader = container->reader();
					int val		  = reader;
				}
			});

		auto thread2 = std::jthread([&](std::stop_token token)
			{
				auto&& writer = container->reader();
				for (size_t i = 0; i < IterationCount; i++)
				{
					auto&& reader = container->reader();
					int val		  = reader;
				}
			});

		thread1.join();
		thread2.join();
		Assert::AreEqual((int) container->reader(), (int) +1);
	}

	TEST_METHOD(TestIterators)
	{
		// No [[nodiscard]] attribute
		// container->writer();
		// container->reader();

		//Test iterators
		auto&& container = _prepare_testing_data();
		auto begin		 = container->writer()->begin();
		auto end		 = container->writer()->end();
		auto size		 = container->writer()->size();

		Assert::AreEqual<size_t>(size, 3);
		Assert::IsFalse(begin == end);
		container->writer()->emplace(20, 20);

		auto beginConst = container->reader()->begin();
		auto endConst	= container->reader()->end();
		size			= container->reader()->size();
		Assert::IsFalse(beginConst == endConst);
		Assert::AreEqual<size_t>(size, 4);

		{ //begin const
			auto begin = container->writer()->begin();

			//beginConst->second++; //atomic_concurrent
			begin->second++; //Writer
			Assert::AreEqual<int>(begin->second, 6);
		}
	}

	TEST_METHOD(TestRangeLoop)
	{
		int result		 = 0;
		auto&& container = _prepare_testing_data();

		for (auto&& item : container->writer())
		{
			result += 5;
			Assert::AreEqual(item.first, result);
		}

		result = 0;
		for (auto&& item : container->reader())
		{
			result += 5;
			Assert::AreEqual(item.first, result);
		}
	}

	TEST_METHOD(TestWriterAccessSynchroAsync)
	{
		auto&& container = _prepare_testing_data();

		std::condition_variable con;
		auto future = std::async(std::launch::async, [&]()
			{
				con.notify_one();
				int i = 5;
				for (auto&& item : container->writer())
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
		for (auto&& item : container->writer())
		{
			Assert::AreEqual(item.second, i);
			i += 5;
			item.second -= 1;
		}

		future.get();
	}

	TEST_METHOD(TestIndexOperator)
	{
		auto&& container = atomic_concurrent::vector<int>();

		//Index operators on unordered map
		{
			auto scope = container.writer();
			Assert::ExpectException<std::out_of_range>([&]()
				{
					[[maybe_unused]] auto result = scope->at(20);
				});

			scope->resize(5);
			scope[0] = 25;
		}

		{
			auto scope = container.reader();
			Assert::AreEqual<const int&>(scope[0], 25);
		}
	}

	TEST_METHOD(TestFunctionCallOperator)
	{
		auto&& container = _prepare_testing_data();
		{
			auto scope = container->writer();
			scope().emplace(30, 30);
			scope().emplace(35, 35);

			Assert::AreEqual(scope[30], 30);
			Assert::AreEqual(scope[35], 35);
		}

		{
			auto scope	= container->reader();
			auto scope2 = container->reader();
			Assert::AreEqual(scope().at(30), 30);
			Assert::AreEqual(scope2().at(35), 35);
		}
	}

	TEST_METHOD(TestSwapSemantics)
	{
		auto&& container = _prepare_testing_data();

		std::unordered_map<int, int> tmpContainer;
		tmpContainer.emplace(30, 30);
		tmpContainer.emplace(35, 35);

		auto scope = container->writer();
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
		auto&& container  = _prepare_testing_data();
		auto&& scope	  = container->writer();
		auto tmpContainer = scope.move();

		Assert::AreEqual<int>(scope.size(), 0);
		Assert::AreEqual<int>(tmpContainer[5], 5);
		Assert::AreEqual<int>(tmpContainer[10], 10);
		Assert::AreEqual<int>(tmpContainer[15], 15);
	}

	TEST_METHOD(TestSetSemantics)
	{
		auto&& container = _prepare_testing_data();

		std::unordered_map<int, int> tmpContainer;
		tmpContainer.emplace(30, 30);
		tmpContainer.emplace(35, 35);

		auto scope = container->writer();
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
		auto&& container = _prepare_testing_data();
		{
			auto scope = container->writer();
			scope.get().emplace(30, 30);
			scope.get().emplace(35, 35);

			Assert::AreEqual(scope[30], 30);
			Assert::AreEqual(scope[35], 35);
		}

		{
			auto scope	= container->reader();
			auto scope2 = container->reader();
			Assert::AreEqual(scope.get().at(30), 30);
			Assert::AreEqual(scope2.get().at(35), 35);
		}
	}

	TEST_METHOD(TestUserDefinedConversion)
	{
		int iCalls		 = 0;
		auto&& container = _prepare_testing_data();
		auto testLambda	 = [&](std::unordered_map<int, int>& container)
		{
			iCalls++;
		};

		auto testLambdaConst = [&](const std::unordered_map<int, int>& container)
		{
			iCalls++;
		};

		testLambda(container->writer());
		testLambdaConst(container->writer());
		//testLambda(container->reader()); //-> cannot be done due constless
		testLambdaConst(container->reader());

		Assert::AreEqual(iCalls, 3);
	}

	//TEST_METHOD(TestSynchronisationSemantics)
	//{
	//	size_t size			 = 0;
	//	size_t notifications = 0;
	//	std::condition_variable_any cv;
	//	auto&& container = _prepare_testing_data();
	//	auto oFuture   = std::async(std::launch::async, [&size, &cv, &container]()
	//		  {
	//			  container->writer().wait(cv);
	//			  container->writer().wait(cv, [&size]()
	//				  {
	//					  return ++size == 2;
	//				  });

	//			  container->reader().wait(cv);
	//			  container->reader().wait(cv, [&size]()
	//				  {
	//					  return ++size == 4;
	//				  });
	//		  });

	//	for (; oFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready;)
	//		cv.notify_one();

	//	oFuture.get();
	//	Assert::AreEqual<size_t>(size, 4);
	//}

	//TEST_METHOD(TestReleaseSemantics)
	//{
	//	auto&& container = _prepare_testing_data();
	//	{
	//		auto scope = container->writer();
	//		Assert::IsTrue(scope().contains(15));
	//		scope.release();

	//		Assert::ExpectException<std::system_error>([&]()
	//			{
	//				[[maybe_unused]] auto result = scope->at(15);
	//			});
	//	}

	//	auto scope = container->reader();
	//	Assert::AreEqual(scope().at(15), 15);
	//	scope.release();
	//	Assert::ExpectException<std::system_error>([&]()
	//		{
	//			[[maybe_unused]] auto result = scope->at(15);
	//		});
	//	Assert::ExpectException<std::system_error>([&]
	//		{
	//			scope.release();
	//		});
	//}

	//TEST_METHOD(TestAcquireSemantics)
	//{
	//	auto&& container = _prepare_testing_data();
	//	{ //Acquire write
	//		auto&& scope = container->writer();
	//		scope->emplace(40, 40);
	//		Assert::AreEqual(scope().at(40), 40);
	//		scope.release();
	//		Assert::ExpectException<std::system_error>([&]()
	//			{
	//				[[maybe_unused]] auto result = scope->at(40);
	//			});
	//		scope.acquire();
	//		Assert::AreEqual(scope().at(40), 40);
	//		scope->emplace(222, 222);
	//		Assert::AreEqual(scope().at(222), 222);

	//		Assert::ExpectException<std::system_error>([&]
	//			{
	//				scope.acquire();
	//			});

	//		scope->emplace(226, 226);
	//		Assert::AreEqual(scope().at(226), 226);
	//	}

	//	{ //Acquire read
	//		auto&& scope = container->reader();
	//		Assert::AreEqual(scope().at(40), 40);
	//		scope.release();

	//		Assert::ExpectException<std::system_error>([&]()
	//			{
	//				scope->at(40);
	//			});
	//		scope.acquire();
	//		Assert::AreEqual(scope().at(40), 40);
	//		scope.release();
	//		container->writer()->emplace(223, 223);

	//		scope.acquire();
	//		Assert::AreEqual(scope().at(223), 223);
	//	}
	//}

	//TEST_METHOD(TestReassign)
	//{
	//	auto&& container = _prepare_testing_data();
	//	//Writer
	//	auto scope(std::move(container->writer()));
	//	Assert::ExpectException<std::system_error>([&]
	//		{
	//			scope.acquire();
	//		});

	//	Assert::AreEqual(scope().at(15), 15);
	//	scope.release();
	//	Assert::ExpectException<std::system_error>([&]
	//		{
	//			scope.release();
	//		});

	//	scope = container->writer();
	//	Assert::ExpectException<std::system_error>([&]
	//		{
	//			scope.acquire();
	//		});

	//	Assert::AreEqual(scope().at(15), 15);
	//	scope.release();
	//	Assert::ExpectException<std::system_error>([&]
	//		{
	//			scope.release();
	//		});

	//	//atomic_concurrent
	//	auto scopeRead(std::move(container->reader()));
	//	Assert::ExpectException<std::system_error>([&]
	//		{
	//			scopeRead.acquire();
	//		});
	//	Assert::AreEqual(scopeRead().at(15), 15);
	//	scopeRead.release();
	//	Assert::ExpectException<std::system_error>([&]
	//		{
	//			scopeRead.release();
	//		});

	//	scopeRead = container->reader();
	//	Assert::ExpectException<std::system_error>([&]
	//		{
	//			scopeRead.acquire();
	//		});
	//	Assert::AreEqual(scopeRead().at(15), 15);
	//	scopeRead.release();
	//	Assert::ExpectException<std::system_error>([&]
	//		{
	//			scopeRead.release();
	//		});
	//}

	//TEST_METHOD(TestObjectLifetime)
	//{
	//	{ //Writer
	//		std::optional<atomic_concurrent::resource_writer<std::unordered_map<int, int>>> scope;
	//		Assert::IsFalse(scope.has_value());

	//		{
	//			auto&& container = _prepare_testing_data();
	//			scope			 = container->writer();
	//			Assert::IsTrue(scope.has_value());
	//			Assert::AreEqual(scope.value()->at(15), 15);
	//		}

	//		Assert::IsTrue(scope.has_value());
	//		Assert::AreEqual(scope.value()->at(15), 15);

	//		/*scope->release();
	//		scope->acquire();*/

	//		Assert::AreEqual(scope.value()->at(15), 15);
	//	}

	//	{ //atomic_concurrent
	//		std::optional<atomic_concurrent::resource_reader<std::unordered_map<int, int>>> scope;
	//		Assert::IsFalse(scope.has_value());

	//		{
	//			auto&& container = _prepare_testing_data();
	//			scope			 = container->reader();
	//			Assert::IsTrue(scope.has_value());
	//			Assert::AreEqual(scope.value()->at(15), 15);
	//		}

	//		Assert::IsTrue(scope.has_value());
	//		Assert::AreEqual(scope.value()->at(15), 15);

	//		/*		scope->release();
	//		scope->acquire();*/

	//		Assert::AreEqual(scope.value()->at(15), 15);
	//	}
	//}

	TEST_METHOD(TestFunctor)
	{
		int iNumber								  = 0;
		atomic_concurrent::functor<void()> fnTest = std::function<void()>([&]()
			{
				iNumber++;
			});

		fnTest.writer().get()();
		fnTest.writer().get()();
		fnTest.reader().get()();

		Assert::AreEqual(3, iNumber);
	}

	TEST_METHOD(TestConccurentConstraints)
	{
		atomic_concurrent::list<int> listNumbers = std::list<int>{
			1,
			2,
			3,
			4,
			5
		};

		//ContainerTest(listNumbers);
	}

	TEST_METHOD(FechAdd)
	{
		constexpr size_t loopCount = 500000;
		auto&& container		   = std::atomic<size_t>(0);
		auto thread1			   = std::jthread([&]()
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
		Assert::AreEqual(loopCount * 2, container.load());
	}

	/*TEST_METHOD(ThreadsSync)
	{
		constexpr size_t loopCount = 500000;
		auto&& container		   = _prepare_testing_data();
		auto thread1			   = std::jthread([&]()
			  {
				  for (size_t i = 0; i < loopCount; i++)
				  {
					  auto writer = container->writer();
					  writer->emplace(5, 5);
				  }
			  });

		auto thread2 = std::jthread([&]()
			{
				for (size_t i = 0; i < loopCount; i++)
				{
					auto writer = container->writer();
					writer->emplace(10, 10);
				}
			});
	}

	TEST_METHOD(ThreadsSync2)
	{
		size_t counter			   = 0;
		constexpr size_t loopCount = 500000;
		auto&& container		   = _prepare_testing_data();
		auto thread1			   = std::jthread([&]()
			  {
				  for (size_t i = 0; i < loopCount; i++)
				  {
					  auto writer = container->writer();
					  if (i % 2 == 0)
						  writer->emplace(5, 5);
					  else
						  writer->erase(5);
				  }
			  });

		auto thread2 = std::jthread([&]()
			{
				for (size_t i = 0; i < loopCount; i++)
				{
					auto writer = container->reader();
					if (writer->contains(5))
						counter++;
				}
			});
		thread1.join();
		thread2.join();
		Assert::AreEqual((size_t) 1, counter);
	}*/
};
} // namespace FrameworkTesting
