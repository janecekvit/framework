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

	storage::heterogeneous_container InitializeHeterogeneousContainer()
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

		return storage::heterogeneous_container(25, 331, 1.1, "string"s, "kase"s, std::make_tuple(25, 333), fnCallbackInt, fnCallbackInt2, fnCallbackString, fnCallbackString2);
	}

	TEST_METHOD(TestClear)
	{
		using namespace std::string_literals;

		auto oContainer = InitializeHeterogeneousContainer();
		Assert::AreEqual(ToList(oContainer.get<int>()), std::list<int>{ 25, 331 });
		Assert::AreEqual(ToList(oContainer.get<std::string>()), std::list<std::string>{ "string", "kase" });

		oContainer.clear<int>();
		Assert::AreEqual(ToList(oContainer.get<int>()), std::list<int>{});
		Assert::AreEqual(ToList(oContainer.get<std::string>()), std::list<std::string>{ "string", "kase" });

		oContainer.clear();
		Assert::AreEqual(ToList(oContainer.get<int>()), std::list<int>{});
		Assert::AreEqual(ToList(oContainer.get<std::string>()).size(), size_t(0));
	}

	TEST_METHOD(TestCallMethods)
	{
		auto oContainer = InitializeHeterogeneousContainer();

		// Test call methods
		int iCallable = 5;
		oContainer.call_first<std::function<void(int&)>>(iCallable);
		Assert::AreEqual(15, iCallable);

		oContainer.call<std::function<void(int&)>>(1, iCallable);
		Assert::AreEqual(35, iCallable);

		std::string sResult = oContainer.call_first<std::function<std::string(std::string&&)>>("Test ");
		Assert::AreEqual("Test 123"s, sResult);

		sResult = oContainer.call<std::function<std::string(std::string&&)>>(1, "Test ");
		Assert::AreEqual("Test 456"s, sResult);

		try
		{
			oContainer.call_first<std::function<void(std::string&&)>>("Test ");
		}
		catch (const std::exception& ex)
		{
			std::string error = "heterogeneous_container: Cannot retrieve value on position 0 with type: "s + typeid(std::function<void(std::string&&)>).name();
			Assert::AreEqual(ex.what(), error.data());
		}

		iCallable = 0;
		oContainer.call_all<std::function<void(int&)>>(iCallable);
		Assert::AreEqual(30, iCallable);

		auto listResults = oContainer.call_all<std::function<std::string(std::string&&)>>("Test ");
		Assert::AreEqual("Test 123"s, *listResults.begin());
		Assert::AreEqual("Test 456"s, *std::next(listResults.begin(), 1));

		// Test size and reset (1)
		Assert::AreEqual(oContainer.size<std::function<void(int&)>>(), size_t(2));
		Assert::AreEqual(oContainer.contains<std::function<void(int&)>>(), true);

		Assert::AreEqual(oContainer.size<std::function<std::string(std::string&&)>>(), size_t(2));
		Assert::AreEqual(oContainer.contains<std::function<std::string(std::string&&)>>(), true);

		// Test size and reset (2)
		oContainer.clear<std::function<void(int&)>>();

		Assert::AreEqual(oContainer.size<std::function<void(int&)>>(), size_t(0));
		Assert::AreEqual(oContainer.contains<std::function<void(int&)>>(), false);

		Assert::AreEqual(oContainer.size<std::function<std::string(std::string&&)>>(), size_t(2));
		Assert::AreEqual(oContainer.contains<std::function<std::string(std::string&&)>>(), true);

		// Test size and reset (3)
		oContainer.clear();

		Assert::AreEqual(oContainer.size<std::function<void(int&)>>(), size_t(0));
		Assert::AreEqual(oContainer.contains<std::function<void(int&)>>(), false);

		Assert::AreEqual(oContainer.size<std::function<std::string(std::string&&)>>(), size_t(0));
		Assert::AreEqual(oContainer.contains<std::function<std::string(std::string&&)>>(), false);
	}

	TEST_METHOD(TestSize)
	{
		auto oContainer = InitializeHeterogeneousContainer();
		Assert::AreEqual(size_t(2), oContainer.size<int>());
		Assert::AreEqual(size_t(1), oContainer.size<double>());
		Assert::AreEqual(size_t(2), oContainer.size<std::string>());
		Assert::AreEqual(size_t(0), oContainer.size<float>());

		Assert::AreEqual(size_t(10), oContainer.size());
	}

	TEST_METHOD(TestEmpty)
	{
		auto oContainer = InitializeHeterogeneousContainer();
		Assert::IsFalse(oContainer.empty<int>());
		Assert::IsFalse(oContainer.empty<double>());
		Assert::IsFalse(oContainer.empty<std::string>());
		Assert::IsTrue(oContainer.empty<float>());

		Assert::IsFalse(oContainer.empty());
	}

	TEST_METHOD(TestContains)
	{
		auto oContainer = InitializeHeterogeneousContainer();
		Assert::IsTrue(oContainer.contains<int>());
		Assert::IsTrue(oContainer.contains<double>());
		Assert::IsTrue(oContainer.contains<std::string>());
		Assert::IsFalse(oContainer.contains<float>());
	}

	TEST_METHOD(TestFirst)
	{
		auto oContainer = InitializeHeterogeneousContainer();
		Assert::AreEqual(oContainer.first<int>(), 25);
		Assert::AreEqual(oContainer.first<std::string>(), "string"s);

		// no item
		Assert::ExpectException<storage::heterogeneous_container::bad_access>([&]()
			{
				std::ignore = oContainer.first<float>();
			});
	}

	TEST_METHOD(TestGet)
	{
		auto oContainer = InitializeHeterogeneousContainer();
		Assert::AreEqual(ToList(oContainer.get<int>()), std::list<int>{ 25, 331 });
		Assert::AreEqual(ToList(oContainer.get<std::string>()), std::list<std::string>{ "string", "kase" });

		oContainer.get<int>(0) = 100;
		oContainer.get<int>(1) = 200;
		Assert::AreEqual(oContainer.get<int>(0), 100);
		Assert::AreEqual(oContainer.get<int>(1), 200);

		auto&& values = oContainer.get<int>();
		values.begin()->get() = 1000;
		std::next(values.begin())->get() = 2000;
		Assert::AreEqual(ToList(oContainer.get<int>()), std::list<int>{ 1000, 2000 });
	}
	TEST_METHOD(TestGetConst)
	{
		const auto oContainer = InitializeHeterogeneousContainer();
		Assert::AreEqual(ToList(oContainer.get<int>()), std::list<int>{ 25, 331 });
		Assert::AreEqual(ToList(oContainer.get<std::string>()), std::list<std::string>{ "string", "kase" });

		Assert::AreEqual(oContainer.get<int>(0), 25);
		Assert::AreEqual(oContainer.get<int>(1), 331);

		// out of range
		Assert::ExpectException<storage::heterogeneous_container::bad_access>([&]()
			{
				std::ignore = oContainer.get<int>(2);
			});

		Assert::AreEqual(oContainer.get<std::string>(0), "string"s);
		Assert::AreEqual(oContainer.get<std::string>(1), "kase"s);
	}

	TEST_METHOD(TestVisitMethods)
	{
		auto oContainer = InitializeHeterogeneousContainer();
		auto oResultInt = ToList(oContainer.get<int>());
		Assert::AreEqual(oResultInt, std::list<int>{ 25, 331 });

		oContainer.visit<int>([&](int& i)
			{
				i += 100;
			});

		oResultInt = ToList(oContainer.get<int>());
		Assert::AreEqual(oResultInt, std::list<int>{ 125, 431 });

		std::list<int> results;
		oContainer.visit<int>([&](const int& i)
			{
				results.emplace_back(i);
			});

		Assert::AreEqual(results, std::list<int>{ 125, 431 });
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

				std::function<void(int&)> fnCallbackInt2 = [this](int& i)
				{
					i += 20;
				};

				m_pContainer = std::make_unique<storage::heterogeneous_container>(fnCallbackInt, fnCallbackInt2);
			}
			virtual ~TestHeterogeneousContainer() = default;

			int call()
			{
				int iCall = 0;
				m_pContainer->call_first<std::function<void(int&)>>(iCall);
				return iCall;
			}

		private:
			std::unique_ptr<storage::heterogeneous_container> m_pContainer = nullptr;
		};

		// Test container in nested class
		TestHeterogeneousContainer oTestContainer;
		Assert::AreEqual(10, oTestContainer.call());
	}

	TEST_METHOD(TestTupleUnpackHeterogeneousContainer)
	{
		auto oHeterogeneousContainer = extensions::tuple::unpack(std::make_tuple(1, 2, 3, "1", "10"));
		Assert::AreEqual(ToList(oHeterogeneousContainer.get<int>()), std::list<int>{ 1, 2, 3 });
		Assert::AreEqual(ToList(oHeterogeneousContainer.get<const char*>()), std::list<const char*>{ "1", "10" });
		auto&& bb = oHeterogeneousContainer.get<int>();
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
			auto result = value.first<int>();
			auto result2 = value.first<std::string>();
		}
	}
};
} // namespace FrameworkTesting
