#include "stdafx.h"

#include "CppUnitTest.h"
#include "storage/heterogeneous_container.h"

#include <fstream>
#include <iostream>
#include <variant>
#include <any>


namespace Microsoft::VisualStudio::CppUnitTestFramework
{

template <>
static std::wstring ToString<std::list<int>>(const std::list<int>& t)
{
	return L"ListInts";
}

template <>
static std::wstring ToString<std::list<const char*>>(const std::list<const char*>& t)
{
	return L"ListInts";
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

	TEST_METHOD(TestGetAndFirstMethods)
	{
		using namespace std::string_literals;

		// Test Get and First methods
		auto oContainer = InitializeHeterogeneousContainer();
		auto oResult	= oContainer.get<std::string>();
		auto oResultInt = oContainer.get<int>();
		Assert::AreEqual(*oResultInt.begin(), 25);
		Assert::AreEqual(*++oResultInt.begin(), 331);

		Assert::AreEqual(oContainer.get<int>(0), 25);
		Assert::AreEqual(oContainer.get<int>(1), 331);
		Assert::AreEqual(oContainer.first<int>(), 25);

		try
		{
			Assert::AreEqual(oContainer.get<int>(2), 331);
		}
		catch (const std::exception& ex)
		{
			Assert::AreEqual(ex.what(), "heterogeneous_container: Cannot retrieve value on position 2 with type: int");
		}

		Assert::AreEqual(oContainer.get<std::string>(0), "string"s);
		Assert::AreEqual(oContainer.get<std::string>(1), "kase"s);
		Assert::AreEqual(oContainer.first<std::string>(), "string"s);
	}
	
	TEST_METHOD(TestVisitMethods)
	{
		auto oContainer = InitializeHeterogeneousContainer();

		// Test visit methods
		oContainer.visit<int>([&](int& i)
			{
				i += 100;
			});

		bool bFirst = true;
		oContainer.visit<int>([&](const int& i)
			{
				Assert::AreEqual(i, bFirst ? 125 : 431);
				bFirst = false;
			});

		auto oResultIntNew = oContainer.get<int>();
		Assert::AreEqual(*oResultIntNew.begin(), 125);
		Assert::AreEqual(*++oResultIntNew.begin(), 431);
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
		Assert::AreEqual(oHeterogeneousContainer.get<int>(), std::list<int>{ 1, 2, 3 });
		Assert::AreEqual(oHeterogeneousContainer.get<const char*>(), std::list<const char*>{ "1", "10" });
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
