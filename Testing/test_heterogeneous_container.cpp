#include "stdafx.h"

#include "CppUnitTest.h"
#include "storage/heterogeneous_container.h"

#include <any>
#include <fstream>
#include <iostream>
#include <numeric>
#include <variant>

namespace Microsoft::VisualStudio::CppUnitTestFramework
{

template <>
static std::wstring ToString<std::list<int>>(const std::list<int>& t)
{
	constexpr auto dashFold = [](const int& a, const int& b)
	{
		return a + b;
	};

	if (t.empty())
		return L"";

	return std::to_wstring(std::accumulate(std::next(t.begin()), t.end(), t.front(), dashFold));
}

template <>
static std::wstring ToString<std::list<const char*>>(const std::list<const char*>& t)
{
	return L"ListInts";
}

template <>
static std::wstring ToString<std::list<std::string>>(const std::list<std::string>& t)
{
	constexpr auto dashFold = [](const std::string& a, const std::string& b)
	{
		return a + '-' + b;
	};

	if (t.empty())
		return L"";

	auto result = std::accumulate(std::next(t.begin()), t.end(), t.front(), dashFold);
	return std::wstring(result.begin(), result.end());
}

} // namespace Microsoft::VisualStudio::CppUnitTestFramework

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace janecekvit;
using namespace std::string_literals;

namespace FrameworkTesting
{

ONLY_USED_AT_NAMESPACE_SCOPE class test_heterogeneous_container : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_heterogeneous_container> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
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
			result.emplace_back((int)value.get());
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
		std::function<void(int&)> fnCallbackInt = [this](int& i)
		{
			i += 10;
		};

		std::function<void(int&)> fnCallbackInt2 = [this](int& i)
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

	TEST_METHOD(TestConstruction)
	{
		storage::heterogeneous_container container(10, 20, 30, 40, "ANO"s, "NE"s, "NEVIM"s, std::make_tuple(50, 60, 70, 80, 90), std::initializer_list<int>{ 100, 110 });
		Assert::AreEqual(ToList(container.get<int>()), std::list<int>{ 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110 });
		Assert::AreEqual(ToList(container.get<std::string>()), std::list<std::string>{ "ANO", "NE", "NEVIM" });
	}

	TEST_METHOD(TestEmplace)
	{
		storage::heterogeneous_container container;
		container.emplace(10, 20, 30, 40, "ANO"s, "NE"s, "NEVIM"s, std::make_tuple(50, 60, 70, 80, 90), std::initializer_list<int>{100, 110});
		Assert::AreEqual(ToList(container.get<int>()), std::list<int>{ 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110 });
		Assert::AreEqual(ToList(container.get<std::string>()), std::list<std::string>{ "ANO", "NE", "NEVIM" });
	}

	TEST_METHOD(TestClear)
	{
		auto container = InitializeHeterogeneousContainer();
		Assert::AreEqual(ToList(container.get<int>()), std::list<int>{ 25, 331 });
		Assert::AreEqual(ToList(container.get<std::string>()), std::list<std::string>{ "string", "kase" });

		container.clear<int>();
		Assert::ExpectException<storage::heterogeneous_container<>::bad_access>([&]()
			{
				std::ignore = container.first<int>();
			});
		Assert::AreEqual(ToList(container.get<std::string>()), std::list<std::string>{ "string", "kase" });

		container.clear();
		Assert::ExpectException<storage::heterogeneous_container<>::bad_access>([&]()
			{
				std::ignore = container.first<int>();
			});
		Assert::ExpectException<storage::heterogeneous_container<>::bad_access>([&]()
			{
				std::ignore = container.first<std::string>();
			});
	}

	TEST_METHOD(TestSize)
	{
		auto container = InitializeHeterogeneousContainer();
		Assert::AreEqual(size_t(2), container.size<int>());
		Assert::AreEqual(size_t(3), container.size<double>());
		Assert::AreEqual(size_t(2), container.size<std::string>());
		Assert::AreEqual(size_t(0), container.size<float>());

		Assert::AreEqual(size_t(7), container.size());
	}

	TEST_METHOD(TestEmpty)
	{
		auto container = InitializeHeterogeneousContainer();
		Assert::IsFalse(container.empty<int>());
		Assert::IsFalse(container.empty<double>());
		Assert::IsFalse(container.empty<std::string>());
		Assert::IsTrue(container.empty<float>());

		Assert::IsFalse(container.empty());
	}

	TEST_METHOD(TestContains)
	{
		auto container = InitializeHeterogeneousContainer();
		Assert::IsTrue(container.contains<int>());
		Assert::IsTrue(container.contains<double>());
		Assert::IsTrue(container.contains<std::string>());
		Assert::IsFalse(container.contains<float>());
	}

	TEST_METHOD(TestFirst)
	{
		auto container = InitializeHeterogeneousContainer();
		container.first<int>() = 40;
		container.first<std::string>() = "XXX";
		container.first<unknown_type_test>() = 100;

		Assert::AreEqual(container.first<int>(), 40);
		Assert::AreEqual(container.first<std::string>(), "XXX"s);
		Assert::AreEqual((int)container.first<unknown_type_test>(), 100);

		// no item
		Assert::ExpectException<storage::heterogeneous_container<>::bad_access>([&]()
			{
				std::ignore = container.first<float>();
			});
	}

	TEST_METHOD(TestFirstConst)
	{
		const auto container = InitializeHeterogeneousContainer();
		Assert::AreEqual(container.first<int>(), 25);
		Assert::AreEqual(container.first<std::string>(), "string"s);
		Assert::AreEqual((int) container.first<unknown_type_test>(), 5);

		// no item
		Assert::ExpectException<storage::heterogeneous_container<>::bad_access>([&]()
			{
				std::ignore = container.first<float>();
			});
	}

	TEST_METHOD(TestGet)
	{
		auto container = InitializeHeterogeneousContainer();
		Assert::AreEqual(ToList(container.get<int>()), std::list<int>{ 25, 331 });
		Assert::AreEqual(ToList(container.get<std::string>()), std::list<std::string>{ "string", "kase" });
		Assert::AreEqual(ToList(container.get<unknown_type_test>()), std::list<int>{ 5, 10 });


		container.get<int>(0) = 100;
		container.get<int>(1) = 200;
		container.get<unknown_type_test>(1) = 500;

		Assert::AreEqual(container.get<int>(0), 100);
		Assert::AreEqual(container.get<int>(1), 200);
		Assert::AreEqual((int)container.get<unknown_type_test>(1), 500);

		auto&& values = container.get<int>();
		values.begin()->get() = 1000;
		std::next(values.begin())->get() = 2000;
		Assert::AreEqual(ToList(container.get<int>()), std::list<int>{ 1000, 2000 });

		auto&& uknownValues = container.get<unknown_type_test>();
		uknownValues.begin()->get() = 1000;
		std::next(uknownValues.begin())->get() = 2000;
		Assert::AreEqual(ToList(container.get<unknown_type_test>()), std::list<int>{ 1000, 2000 });
	}

	TEST_METHOD(TestGetConst)
	{
		const auto container = InitializeHeterogeneousContainer();
		Assert::AreEqual(ToList(container.get<int>()), std::list<int>{ 25, 331 });
		Assert::AreEqual(ToList(container.get<std::string>()), std::list<std::string>{ "string", "kase" });
		Assert::AreEqual(ToList(container.get<unknown_type_test>()), std::list<int>{ 5, 10 });

		Assert::AreEqual(container.get<int>(0), 25);
		Assert::AreEqual(container.get<int>(1), 331);
		Assert::AreEqual((int) container.get<unknown_type_test>(1), 10);

		// out of range
		Assert::ExpectException<storage::heterogeneous_container<>::bad_access>([&]()
			{
				std::ignore = container.get<int>(2);
			});

		Assert::AreEqual(container.get<std::string>(0), "string"s);
		Assert::AreEqual(container.get<std::string>(1), "kase"s);
	}

	TEST_METHOD(TestVisit)
	{
		auto container = InitializeHeterogeneousContainer();
		Assert::AreEqual(ToList(container.get<int>()), std::list<int>{ 25, 331 });
		Assert::AreEqual(ToList(container.get<unknown_type_test>()), std::list<int>{ 5, 10 });

		container.visit<int>([&](int& i)
			{
				i += 100;
			});
		Assert::AreEqual(ToList(container.get<int>()), std::list<int>{ 125, 431 });

		container.visit<unknown_type_test>([&](unknown_type_test& i)
			{
				i += 100;
			});
		Assert::AreEqual(ToList(container.get<unknown_type_test>()), std::list<int>{ 105, 110 });
	}

	TEST_METHOD(TestVisitConst)
	{
		const auto container = InitializeHeterogeneousContainer();
		Assert::AreEqual(ToList(container.get<int>()), std::list<int>{ 25, 331 });
		Assert::AreEqual(ToList(container.get<unknown_type_test>()), std::list<int>{ 5, 10 });

		std::list<int> results;
		container.visit<int>([&](const int& i)
			{
				results.emplace_back(i);
			});
		Assert::AreEqual(results, std::list<int>{ 25, 331 });

		std::list<int> resultsUnknown;
		container.visit<unknown_type_test>([&](const unknown_type_test& i)
			{
				resultsUnknown.emplace_back(i);
			});
		Assert::AreEqual(resultsUnknown, std::list<int>{ 5, 10 });

	}

	TEST_METHOD(TestCallMethods)
	{
		auto container = InitializeHeterogeneousContainer();

		// Test call methods
		int result = 5;
		container.call_first<std::function<void(int&)>>(result);
		Assert::AreEqual(15, result);

		container.call<std::function<void(int&)>>(1, result);
		Assert::AreEqual(35, result);

		std::string sResult = container.call_first<std::function<std::string(std::string&&)>>("Test ");
		Assert::AreEqual("Test 123"s, sResult);

		sResult = container.call<std::function<std::string(std::string&&)>>(1, "Test ");
		Assert::AreEqual("Test 456"s, sResult);

		Assert::ExpectException<storage::heterogeneous_container<>::bad_access>([&]()
			{
				container.call_first<std::function<void(std::string&&)>>("Test ");
			});


		result = 0;
		container.call_all<std::function<void(int&)>>(result);
		Assert::AreEqual(30, result);

		auto listResults = container.call_all<std::function<std::string(std::string&&)>>("Test ");
		Assert::AreEqual(listResults, std::list<std::string>{ "Test 123", "Test 456" });

		// Test size and reset (1)
		Assert::AreEqual(container.size<std::function<void(int&)>>(), size_t(2));
		Assert::AreEqual(container.contains<std::function<void(int&)>>(), true);

		Assert::AreEqual(container.size<std::function<std::string(std::string&&)>>(), size_t(2));
		Assert::AreEqual(container.contains<std::function<std::string(std::string&&)>>(), true);

		// Test size and reset (2)
		container.clear<std::function<void(int&)>>();

		Assert::AreEqual(container.size<std::function<void(int&)>>(), size_t(0));
		Assert::AreEqual(container.contains<std::function<void(int&)>>(), false);

		Assert::AreEqual(container.size<std::function<std::string(std::string&&)>>(), size_t(2));
		Assert::AreEqual(container.contains<std::function<std::string(std::string&&)>>(), true);

		// Test size and reset (3)
		container.clear();

		Assert::AreEqual(container.size<std::function<void(int&)>>(), size_t(0));
		Assert::AreEqual(container.contains<std::function<void(int&)>>(), false);

		Assert::AreEqual(container.size<std::function<std::string(std::string&&)>>(), size_t(0));
		Assert::AreEqual(container.contains<std::function<std::string(std::string&&)>>(), false);
	}

	TEST_METHOD(TestContainerInClass)
	{
		class TestHeterogeneousContainer
		{
		public:
			TestHeterogeneousContainer()
			{
				std::function<void(int&)> fnCallbackInt = [this](int& i)
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
		Assert::AreEqual(10, oTestContainer.call());
	}

	TEST_METHOD(TestTupleUnpack)
	{
		auto container = storage::heterogeneous_container(std::make_tuple(1, 2, 3, "1", "10"));
		Assert::AreEqual(ToList(container.get<int>()), std::list<int>{ 1, 2, 3 });
		Assert::AreEqual(ToList(container.get<const char*>()), std::list<const char*>{ "1", "10" });
	}

	TEST_METHOD(TestInitializerListUnpack)
	{
		auto container = storage::heterogeneous_container(std::initializer_list<int>{ 1, 2, 3 });
		Assert::AreEqual(ToList(container.get<int>()), std::list<int>{ 1, 2, 3 });
	}

	TEST_METHOD(TestUserDefinedTypes)
	{
		static_assert(storage::heterogeneous_container<>::IsKnownType<int>, "T must be integral type.");
		static_assert(!storage::heterogeneous_container<>::IsKnownType<std::shared_ptr<int>>, "T must be shared_ptr type.");

		static_assert(storage::heterogeneous_container<std::shared_ptr<int>, std::shared_ptr<char>>::IsKnownType<int>, "T must be integral type.");
		static_assert(storage::heterogeneous_container<std::shared_ptr<int>, std::shared_ptr<char>>::IsKnownType<std::shared_ptr<int>>, "T must be shared_ptr type.");
		auto container = storage::heterogeneous_container<std::shared_ptr<int>, std::shared_ptr<char>>(std::make_tuple(1, 2, 3, "1", "10"), std::make_shared<int>(5));
		Assert::AreEqual(*container.first<std::shared_ptr<int>>(), 5);
	}

	TEST_METHOD(TestIteratorsUserDefinedTypes)
	{
		int count = 0;
		int intCount = 0;
		int unknownTypeCount = 0;
		auto container = InitializeHeterogeneousContainer();
		for (auto&& item : container)
		{
			if (item.is_type<int>())
				intCount++;

			if (item.is_type<unknown_type_test>())
				unknownTypeCount++;

			count++;
		}

		Assert::AreEqual(count, 13);
		Assert::AreEqual(intCount, 2);
		Assert::AreEqual(unknownTypeCount, 2);
	}

	TEST_METHOD(PerformanceVariant)
	{
		std::variant<int, std::string> value = 5;
		std::variant<int, std::string> value2 = std::string("ANO");

		for (size_t i = 0; i < N; i++)
		{
			int& result = std::get<int>(value);
			std::string& result2 = std::get<std::string>(value2);
		}
	}

	TEST_METHOD(PerformanceAnyCast)
	{
		std::any value = 5;
		std::any value2 = std::string("ANO");
		for (size_t i = 0; i < N; i++)
		{
			auto result = std::any_cast<int&>(value);
			auto result2 = std::any_cast<std::string&>(value2);
		}
	}

	TEST_METHOD(PerformanceHeterogenousContainer)
	{
		storage::heterogeneous_container value(5, std::string("ANO"));
		for (size_t i = 0; i < N; i++)
		{
			auto& result = value.first<int>();
			auto& result2 = value.first<std::string>();
		}
	}
};
} // namespace FrameworkTesting
