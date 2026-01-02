#include "storage/heterogeneous_container.h"

#include <any>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <numeric>
#include <variant>

using namespace janecekvit;
using namespace std::string_literals;

namespace framework_tests
{

class test_heterogeneous_container : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}

public:
#ifdef _DEBUG
	constexpr static const size_t N = 1'000'000;
#else
	constexpr static const size_t N = 100'000'000;
#endif // DEBUG

	struct unknown_type_test
	{
		int _value = 0;

		unknown_type_test(int value)
			: _value(value)
		{
		}

		constexpr int operator()()
		{
			return _value;
		}

		constexpr operator int() const
		{
			return _value;
		}

		constexpr operator int&()
		{
			return _value;
		}
	};

	std::list<int> ToList(const std::list<std::reference_wrapper<unknown_type_test>>& values)
	{
		std::list<int> result;
		for (const auto& value : values)
			result.emplace_back((int) value.get());
		return result;
	}

	std::list<int> ToList(const std::list<std::reference_wrapper<const unknown_type_test>>& values)
	{
		std::list<int> result;
		for (const auto& value : values)
			result.emplace_back((int) value.get());
		return result;
	}

	template <typename _T>
	std::list<_T> ToList(const std::list<std::reference_wrapper<const _T>>& values)
	{
		std::list<_T> result;
		for (const auto& value : values)
			result.emplace_back(value);
		return result;
	}

	std::list<std::string> ToList(const std::list<std::reference_wrapper<const char*>>& values)
	{
		std::list<std::string> result;
		for (const auto& value : values)
			result.emplace_back(value);
		return result;
	}

	template <typename _T>
	std::list<_T> ToList(const std::list<std::reference_wrapper<_T>>& values)
	{
		std::list<_T> result;
		for (const auto& value : values)
			result.emplace_back(value);
		return result;
	}

	storage::heterogeneous_container<> InitializeHeterogeneousContainer()
	{
		std::function<void(int&)> fnCallbackInt = [](int& i)
		{
			i += 10;
		};

		std::function<void(int&)> fnCallbackInt2 = [](int& i)
		{
			i += 20;
		};

		std::function<std::string(std::string&&)> fnCallbackString = [](std::string&& s)
		{
			s += "123";
			return s;
		};

		std::function<std::string(std::string&&)> fnCallbackString2 = [](std::string&& s)
		{
			s += "456";
			return s;
		};

		return storage::heterogeneous_container(25, 331, 1.1, "string"s, "kase"s, std::make_tuple(25.1, 333.1), fnCallbackInt, fnCallbackInt2, fnCallbackString, fnCallbackString2, unknown_type_test(5), unknown_type_test(10));
	}
};

TEST_F(test_heterogeneous_container, TestConstruction)
{
	storage::heterogeneous_container container(10, 20, 30, 40, "ANO"s, "NE"s, "NEVIM"s, std::make_tuple(50, 60, 70, 80, 90), std::initializer_list<int>{ 100, 110 });
	ASSERT_EQ(ToList(container.get<int>()), (std::list<int>{ 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110 }));
	ASSERT_EQ(ToList(container.get<std::string>()), (std::list<std::string>{ "ANO", "NE", "NEVIM" }));
}

TEST_F(test_heterogeneous_container, TestEmplace)
{
	storage::heterogeneous_container container;
	container.emplace(10, 20, 30, 40, "ANO"s, "NE"s, "NEVIM"s, std::make_tuple(50, 60, 70, 80, 90), std::initializer_list<int>{ 100, 110 });
	ASSERT_EQ(ToList(container.get<int>()), (std::list<int>{ 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110 }));
	ASSERT_EQ(ToList(container.get<std::string>()), (std::list<std::string>{ "ANO", "NE", "NEVIM" }));
}

TEST_F(test_heterogeneous_container, TestClear)
{
	auto container = InitializeHeterogeneousContainer();
	ASSERT_EQ(ToList(container.get<int>()), (std::list<int>{ 25, 331 }));
	ASSERT_EQ(ToList(container.get<std::string>()), (std::list<std::string>{ "string", "kase" }));

	container.clear<int>();
	ASSERT_THROW(
		{
			std::ignore = container.first<int>();
		},
		storage::heterogeneous_container<>::bad_access);
	ASSERT_EQ(ToList(container.get<std::string>()), (std::list<std::string>{ "string", "kase" }));

	container.clear();
	ASSERT_THROW(
		{
			std::ignore = container.first<int>();
		},
		storage::heterogeneous_container<>::bad_access);
	ASSERT_THROW(
		{
			std::ignore = container.first<std::string>();
		},
		storage::heterogeneous_container<>::bad_access);
}

TEST_F(test_heterogeneous_container, TestSize)
{
	auto container = InitializeHeterogeneousContainer();
	ASSERT_EQ(size_t(2), container.size<int>());
	ASSERT_EQ(size_t(3), container.size<double>());
	ASSERT_EQ(size_t(2), container.size<std::string>());
	ASSERT_EQ(size_t(0), container.size<float>());

	ASSERT_EQ(size_t(13), container.size());
}

TEST_F(test_heterogeneous_container, TestEmpty)
{
	auto container = InitializeHeterogeneousContainer();
	ASSERT_FALSE(container.empty<int>());
	ASSERT_FALSE(container.empty<double>());
	ASSERT_FALSE(container.empty<std::string>());
	ASSERT_TRUE(container.empty<float>());

	ASSERT_FALSE(container.empty());
}

TEST_F(test_heterogeneous_container, TestContains)
{
	auto container = InitializeHeterogeneousContainer();
	ASSERT_TRUE(container.contains<int>());
	ASSERT_TRUE(container.contains<double>());
	ASSERT_TRUE(container.contains<std::string>());
	ASSERT_FALSE(container.contains<float>());
}

TEST_F(test_heterogeneous_container, TestFirst)
{
	auto container = InitializeHeterogeneousContainer();
	container.first<int>() = 40;
	container.first<std::string>() = "XXX";
	container.first<unknown_type_test>() = 100;

	ASSERT_EQ(container.first<int>(), 40);
	ASSERT_EQ(container.first<std::string>(), "XXX"s);
	ASSERT_EQ((int) container.first<unknown_type_test>(), 100);

	// no item
	ASSERT_THROW(
		{
			std::ignore = container.first<float>();
		},
		storage::heterogeneous_container<>::bad_access);
}

TEST_F(test_heterogeneous_container, TestFirstConst)
{
	const auto container = InitializeHeterogeneousContainer();
	ASSERT_EQ(container.first<int>(), 25);
	ASSERT_EQ(container.first<std::string>(), "string"s);
	ASSERT_EQ((int) container.first<unknown_type_test>(), 5);

	// no item
	ASSERT_THROW(
		{
			std::ignore = container.first<float>();
		},
		storage::heterogeneous_container<>::bad_access);
}

TEST_F(test_heterogeneous_container, TestGet)
{
	auto container = InitializeHeterogeneousContainer();
	ASSERT_EQ(ToList(container.get<int>()), (std::list<int>{ 25, 331 }));
	ASSERT_EQ(ToList(container.get<std::string>()), (std::list<std::string>{ "string", "kase" }));
	ASSERT_EQ(ToList(container.get<unknown_type_test>()), (std::list<int>{ 5, 10 }));

	container.get<int>(0) = 100;
	container.get<int>(1) = 200;
	container.get<unknown_type_test>(1) = 500;

	ASSERT_EQ(container.get<int>(0), 100);
	ASSERT_EQ(container.get<int>(1), 200);
	ASSERT_EQ((int) container.get<unknown_type_test>(1), 500);

	auto&& values = container.get<int>();
	values.begin()->get() = 1000;
	std::next(values.begin())->get() = 2000;
	ASSERT_EQ(ToList(container.get<int>()), (std::list<int>{ 1000, 2000 }));

	auto&& uknownValues = container.get<unknown_type_test>();
	uknownValues.begin()->get() = 1000;
	std::next(uknownValues.begin())->get() = 2000;
	ASSERT_EQ(ToList(container.get<unknown_type_test>()), (std::list<int>{ 1000, 2000 }));
}

TEST_F(test_heterogeneous_container, TestGetConst)
{
	const auto container = InitializeHeterogeneousContainer();
	ASSERT_EQ(ToList(container.get<int>()), (std::list<int>{ 25, 331 }));
	ASSERT_EQ(ToList(container.get<std::string>()), (std::list<std::string>{ "string", "kase" }));
	ASSERT_EQ(ToList(container.get<unknown_type_test>()), (std::list<int>{ 5, 10 }));

	ASSERT_EQ(container.get<int>(0), 25);
	ASSERT_EQ(container.get<int>(1), 331);
	ASSERT_EQ((int) container.get<unknown_type_test>(1), 10);

	// out of range
	ASSERT_THROW(
		{
			std::ignore = container.get<int>(2);
		},
		storage::heterogeneous_container<>::bad_access);

	ASSERT_EQ(container.get<std::string>(0), "string"s);
	ASSERT_EQ(container.get<std::string>(1), "kase"s);
}

TEST_F(test_heterogeneous_container, TestVisit)
{
	auto container = InitializeHeterogeneousContainer();
	ASSERT_EQ(ToList(container.get<int>()), (std::list<int>{ 25, 331 }));
	ASSERT_EQ(ToList(container.get<unknown_type_test>()), (std::list<int>{ 5, 10 }));

	container.visit<int>([&](int& i)
		{
			i += 100;
		});
	ASSERT_EQ(ToList(container.get<int>()), (std::list<int>{ 125, 431 }));

	container.visit<unknown_type_test>([&](unknown_type_test& i)
		{
			i += 100;
		});
	ASSERT_EQ(ToList(container.get<unknown_type_test>()), (std::list<int>{ 105, 110 }));
}

TEST_F(test_heterogeneous_container, TestVisitConst)
{
	const auto container = InitializeHeterogeneousContainer();
	ASSERT_EQ(ToList(container.get<int>()), (std::list<int>{ 25, 331 }));
	ASSERT_EQ(ToList(container.get<unknown_type_test>()), (std::list<int>{ 5, 10 }));

	std::list<int> results;
	container.visit<int>([&](const int& i)
		{
			results.emplace_back(i);
		});
	ASSERT_EQ(results, (std::list<int>{ 25, 331 }));

	std::list<int> resultsUnknown;
	container.visit<unknown_type_test>([&](const unknown_type_test& i)
		{
			resultsUnknown.emplace_back(i);
		});
	ASSERT_EQ(resultsUnknown, (std::list<int>{ 5, 10 }));
}

TEST_F(test_heterogeneous_container, TestCallMethods)
{
	auto container = InitializeHeterogeneousContainer();

	// Test call methods
	int result = 5;
	container.call_first<std::function<void(int&)>>(result);
	ASSERT_EQ(15, result);

	container.call<std::function<void(int&)>>(1, result);
	ASSERT_EQ(35, result);

	std::string sResult = container.call_first<std::function<std::string(std::string&&)>>("Test ");
	ASSERT_EQ("Test 123"s, sResult);

	sResult = container.call<std::function<std::string(std::string&&)>>(1, "Test ");
	ASSERT_EQ("Test 456"s, sResult);

	ASSERT_THROW(
		{
			container.call_first<std::function<void(std::string&&)>>("Test ");
		},
		storage::heterogeneous_container<>::bad_access);

	result = 0;
	container.call_all<std::function<void(int&)>>(result);
	ASSERT_EQ(30, result);

	auto listResults = container.call_all<std::function<std::string(std::string&&)>>("Test ");
	ASSERT_EQ(listResults, (std::list<std::string>{ "Test 123", "Test 456" }));

	// Test size and reset (1)
	ASSERT_EQ(container.size<std::function<void(int&)>>(), size_t(2));
	ASSERT_EQ(container.contains<std::function<void(int&)>>(), true);

	ASSERT_EQ(container.size<std::function<std::string(std::string&&)>>(), size_t(2));
	ASSERT_EQ(container.contains<std::function<std::string(std::string&&)>>(), true);

	// Test size and reset (2)
	container.clear<std::function<void(int&)>>();

	ASSERT_EQ(container.size<std::function<void(int&)>>(), size_t(0));
	ASSERT_EQ(container.contains<std::function<void(int&)>>(), false);

	ASSERT_EQ(container.size<std::function<std::string(std::string&&)>>(), size_t(2));
	ASSERT_EQ(container.contains<std::function<std::string(std::string&&)>>(), true);

	// Test size and reset (3)
	container.clear();

	ASSERT_EQ(container.size<std::function<void(int&)>>(), size_t(0));
	ASSERT_EQ(container.contains<std::function<void(int&)>>(), false);

	ASSERT_EQ(container.size<std::function<std::string(std::string&&)>>(), size_t(0));
	ASSERT_EQ(container.contains<std::function<std::string(std::string&&)>>(), false);
}

TEST_F(test_heterogeneous_container, TestContainerInClass)
{
	class TestHeterogeneousContainer
	{
	public:
		TestHeterogeneousContainer()
		{
			std::function<void(int&)> fnCallbackInt = [](int& i)
			{
				i += 10;
			};

			m_pContainer = std::make_unique<storage::heterogeneous_container<>>(fnCallbackInt);
		}

		virtual ~TestHeterogeneousContainer() = default;

		int call()
		{
			int iCall = 0;
			m_pContainer->call_first<std::function<void(int&)>>(iCall);
			return iCall;
		}

	private:
		std::unique_ptr<storage::heterogeneous_container<>> m_pContainer = nullptr;
	};

	// Test container in nested class
	TestHeterogeneousContainer oTestContainer;
	ASSERT_EQ(10, oTestContainer.call());
}

TEST_F(test_heterogeneous_container, TestTupleUnpack)
{
	auto container = storage::heterogeneous_container(std::make_tuple(1, 2, 3, "1", "10"));

	ASSERT_EQ(ToList(container.get<int>()), (std::list<int>{ 1, 2, 3 }));
	ASSERT_EQ(ToList(container.get<const char*>()), (std::list<std::string>{ "1", "10" }));
}

TEST_F(test_heterogeneous_container, TestInitializerListUnpack)
{
	auto container = storage::heterogeneous_container(std::initializer_list<int>{ 1, 2, 3 });
	ASSERT_EQ(ToList(container.get<int>()), (std::list<int>{ 1, 2, 3 }));
}

TEST_F(test_heterogeneous_container, TestUserDefinedTypes)
{
	static_assert(storage::heterogeneous_container<>::is_known_type<int>, "T must be integral type.");
	static_assert(!storage::heterogeneous_container<>::is_known_type<std::shared_ptr<int>>, "T must be shared_ptr type.");

	static_assert(storage::heterogeneous_container<std::shared_ptr<int>, std::shared_ptr<char>>::is_known_type<int>, "T must be integral type.");
	static_assert(storage::heterogeneous_container<std::shared_ptr<int>, std::shared_ptr<char>>::is_known_type<std::shared_ptr<int>>, "T must be shared_ptr type.");
	auto container = storage::heterogeneous_container<std::shared_ptr<int>, std::shared_ptr<char>>(std::make_tuple(1, 2, 3, "1", "10"), std::make_shared<int>(5));
	ASSERT_EQ(*container.first<std::shared_ptr<int>>(), 5);
}

TEST_F(test_heterogeneous_container, TestIterators)
{
	int count = 0;
	int intCount = 0;
	auto container = InitializeHeterogeneousContainer();
	for (auto&& item : container)
	{
		if (item.is_type<int>())
		{
			item.get<int>() += 100;
			intCount++;
		}

		count++;
	}

	ASSERT_EQ(count, 13);
	ASSERT_EQ(intCount, 2);
	ASSERT_EQ(ToList(container.get<int>()), (std::list<int>{ 125, 431 }));
}

TEST_F(test_heterogeneous_container, TestIteratorsConst)
{
	int count = 0;
	int intCount = 0;
	int unknownTypeCount = 0;
	const auto container = InitializeHeterogeneousContainer();
	for (auto&& item : container)
	{
		if (item.is_type<int>())
			intCount++;

		if (item.is_type<unknown_type_test>())
			unknownTypeCount++;

		count++;
	}

	ASSERT_EQ(count, 13);
	ASSERT_EQ(intCount, 2);
	ASSERT_EQ(unknownTypeCount, 2);
}

TEST_F(test_heterogeneous_container, PerformanceVariant)
{
	std::variant<int, std::string> value = 5;
	std::variant<int, std::string> value2 = std::string("ANO");

	for (size_t i = 0; i < N; i++)
	{
		[[maybe_unused]] int& result = std::get<int>(value);
		[[maybe_unused]] std::string& result2 = std::get<std::string>(value2);
	}
}

TEST_F(test_heterogeneous_container, PerformanceAnyCast)
{
	std::any value = 5;
	std::any value2 = std::string("ANO");
	for (size_t i = 0; i < N; i++)
	{
		[[maybe_unused]] auto result = std::any_cast<int&>(value);
		[[maybe_unused]] auto result2 = std::any_cast<std::string&>(value2);
	}
}

TEST_F(test_heterogeneous_container, PerformanceHeterogenousContainer)
{
	storage::heterogeneous_container value(5, std::string("ANO"));
	auto int_refs = value.get<int>();
	auto str_refs = value.get<std::string>();
	auto& int_ref = int_refs.front().get();
	auto& str_ref = str_refs.front().get();

	for (size_t i = 0; i < N; i++)
	{
		[[maybe_unused]] auto& result = int_ref;
		[[maybe_unused]] auto& result2 = str_ref;
	}
}

TEST_F(test_heterogeneous_container, TestEmptyContainerEdgeCases)
{
	storage::heterogeneous_container<> container;
	ASSERT_TRUE(container.empty());
	ASSERT_EQ(container.size(), 0);
	ASSERT_EQ(container.begin(), container.end());
}

TEST_F(test_heterogeneous_container, TestContainerEdgeCases)
{
	storage::heterogeneous_container<> container;
	ASSERT_THROW(std::ignore = container.first<int>(), storage::heterogeneous_container<>::bad_access);
	ASSERT_THROW(std::ignore = container.get<int>(0), storage::heterogeneous_container<>::bad_access);
	ASSERT_FALSE(container.contains<int>());
}

TEST_F(test_heterogeneous_container, TestMoveSemantics)
{
	auto c1 = InitializeHeterogeneousContainer();
	ASSERT_EQ(c1.size<int>(), 2);

	// Move constructor
	auto c2 = std::move(c1);
	ASSERT_EQ(c2.size<int>(), 2);
	ASSERT_EQ(ToList(c2.get<int>()), (std::list<int>{ 25, 331 }));
	ASSERT_EQ(c1.size<int>(), 0);

	// Move assignment
	storage::heterogeneous_container<> c3;
	c3 = std::move(c2);
	ASSERT_EQ(c3.size<int>(), 2);
	ASSERT_EQ(c2.size<int>(), 0);
}

TEST_F(test_heterogeneous_container, TestConstIterators)
{
	const auto container = InitializeHeterogeneousContainer();

	int count = 0;
	for (const auto& item : container)
	{
		if (item.is_type<int>())
		{
			[[maybe_unused]] const auto& value = item.get<int>();
		}
		count++;
	}

	ASSERT_EQ(count, 13);
	ASSERT_EQ(ToList(container.get<int>()), (std::list<int>{ 25, 331 }));
}

TEST_F(test_heterogeneous_container, TestIteratorIncrementEdgeCases)
{
	storage::heterogeneous_container<> container(1, 2, 3);

	auto it = container.begin();
	int count = 0;
	while (it != container.end())
	{
		++it;
		count++;
	}
	ASSERT_EQ(count, 3);
}

TEST_F(test_heterogeneous_container, TestLargeDatasetPerformance)
{
	storage::heterogeneous_container<> container;

	for (int i = 0; i < 10000; ++i)
	{
		container.emplace(i, std::string("str") + std::to_string(i), double(i) * 1.5);
	}

	ASSERT_EQ(container.size<int>(), 10000);
	ASSERT_EQ(container.size<std::string>(), 10000);
	ASSERT_EQ(container.size<double>(), 10000);

	int count = 0;
	for ([[maybe_unused]] const auto& item : container)
	{
		count++;
	}
	ASSERT_EQ(count, 30000);
}

TEST_F(test_heterogeneous_container, TestTypeKeyStability)
{
	storage::heterogeneous_container<> c1, c2;

	c1.emplace(42);
	c2.emplace(99);

	ASSERT_TRUE(c1.contains<int>());
	ASSERT_TRUE(c2.contains<int>());
	ASSERT_EQ(c1.first<int>(), 42);
	ASSERT_EQ(c2.first<int>(), 99);
}

TEST_F(test_heterogeneous_container, TestTypeReserve)
{
	storage::heterogeneous_container<> c1;

	c1.reserve<int>(5);
	c1.emplace(42);
	ASSERT_TRUE(c1.contains<int>());
	ASSERT_EQ(c1.first<int>(), 42);
}

} // namespace framework_tests
