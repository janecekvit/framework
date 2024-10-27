#include "stdafx.h"

#include "CppUnitTest.h"
#include "extensions/cloneable.h"
#include "extensions/extensions.h"
#include "storage/heterogeneous_container.h"
#include "synchronization/concurrent.h"

#include <future>
#include <iostream>
#include <string>
#include <thread>
#include <ranges>


#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT_64
#else
#define ENVIRONMENT_32
#endif
#endif

using namespace janecekvit;

namespace Microsoft::VisualStudio::CppUnitTestFramework
{

// TypeDefs
template <>
static std::wstring ToString<std::tuple<int, int, int, int, int, int, int, int, int, int>>(const std::tuple<int, int, int, int, int, int, int, int, int, int>& t)
{
	return L"TupleInts";
}

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

using namespace std::string_literals;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
// https://docs.microsoft.com/cs-cz/visualstudio/test/microsoft-visualstudio-testtools-cppunittestframework-api-reference?view=vs-2019

////////////////////////////////////////////////////////////////////////////////////////////

struct impl : public virtual cloneable<impl>
{
	std::unique_ptr<impl> clone() const override
	{
		return std::make_unique<impl>(*this);
	}
};

class Ports
{
public:
	unsigned int uSourcePort, uDestinationPort;

	Ports(_In_ unsigned int uSPort, _In_ unsigned int uDPort)
		: uSourcePort(uSPort)
		, uDestinationPort(uDPort)
	{
		m_mapINT.emplace(uSPort, uDPort);
	}

	void Compute() const
	{
		auto jj = extensions::execute_on_container(m_mapINT, 5, [](_In_ auto& oResult)
			{
				return oResult;
			});
	}

private:
	std::unordered_map<int, int> m_mapINT;
};

struct PortsCmp
{
	size_t operator()(_In_ const Ports& x) const
	{
		return (std::hash<int>()(x.uSourcePort) ^ std::hash<int>()(x.uDestinationPort));
	}

	bool operator()(_In_ const Ports& a, _In_ const Ports& b) const
	{
		return a.uSourcePort == b.uSourcePort && a.uDestinationPort == b.uDestinationPort;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////

namespace FrameworkTesting
{
ONLY_USED_AT_NAMESPACE_SCOPE class test_extensions : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_extensions> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
public:
	TEST_METHOD(TestExecuteOnContainerObject)
	{
		std::unordered_map<Ports, std::string, PortsCmp, PortsCmp> myMap = { { Ports(5, 6), "tezko" } };

		auto value1 = extensions::execute_on_container(myMap, Ports(5, 6), [](std::string& oResult)
			{
				return 20;
			});

		auto value2 = extensions::execute_on_container(myMap, Ports(5, 7), [](std::string& oResult)
			{
				return 10;
			});

		Assert::AreEqual(value1, 20);
		Assert::AreEqual(value2, 0);
	}
	TEST_METHOD(TestExecuteOnContainerObjectConst)
	{
		const std::unordered_map<Ports, std::string, PortsCmp, PortsCmp> myMap = { { Ports(5, 6), "tezko" } };

		auto value1 = extensions::execute_on_container(myMap, Ports(5, 6), [](const std::string& oResult)
			{
				return 20;
			});

		auto value2 = extensions::execute_on_container(myMap, Ports(5, 7), [](const std::string& oResult)
			{
				return 10;
			});

		Assert::AreEqual(value1, 20);
		Assert::AreEqual(value2, 0);
	}

	TEST_METHOD(TestRecast)
	{
		struct base
		{
			virtual ~base() = default;
		};

		struct derived : public base
		{
		};

		auto ptr = std::make_unique<derived>();
		Assert::IsTrue(static_cast<bool>(ptr));

		auto ptr2 = extensions::recast<derived, base>(std::move(ptr));
#pragma warning(suppress : 26800)
		Assert::IsFalse(static_cast<bool>(ptr));
		Assert::IsTrue(static_cast<bool>(ptr2));

		auto ptr3 = extensions::recast<base, derived>(std::move(ptr2));
		Assert::IsFalse(static_cast<bool>(ptr));
#pragma warning(suppress : 26800)
		Assert::IsFalse(static_cast<bool>(ptr2));
		Assert::IsTrue(static_cast<bool>(ptr3));
	}

	TEST_METHOD(TestTuplePrint)
	{
		auto oStream = extensions::tuple::print(std::make_tuple(1, 2, 3, "1", "10"), std::string(", "));
		Assert::AreEqual(oStream.str(), "1, 2, 3, 1, 10, "s);
	}

	TEST_METHOD(TestTupleGenerator)
	{
		auto fnCallback = [](auto&&... oArgs) -> int
		{
			auto tt = std::forward_as_tuple(oArgs...);
			return std::get<0>(tt);
		};
		auto oResultGenerator = extensions::tuple::generate<10>(fnCallback);
		Assert::AreEqual(oResultGenerator, std::make_tuple(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));
	}

	TEST_METHOD(TestTupleUnpackHeterogeneousContainer)
	{
		auto oHeterogeneousContainer = extensions::tuple::unpack(std::make_tuple(1, 2, 3, "1", "10"));
		Assert::AreEqual(oHeterogeneousContainer.get<int>(), std::list<int>{ 1, 2, 3 });
		Assert::AreEqual(oHeterogeneousContainer.get<const char*>(), std::list<const char*>{ "1", "10" });
	}

	TEST_METHOD(TestNumeric)
	{
		auto result = extensions::numeric::factorial<5>();
		Assert::AreEqual(result.value, (size_t) 120);
	}

	TEST_METHOD(TestHashFunction)
	{
		std::string s("ano");
		int i		 = 5;
		size_t uHash = extensions::hash::combine(s, i);

#if defined ENVIRONMENT_64
		Assert::AreEqual(uHash, static_cast<size_t>(8002369318281051212));
#elif defined ENVIRONMENT_32
		Assert::AreEqual(uHash, static_cast<size_t>(730160148));
#endif
	}

	///////////////////////////////////////////////////////////////////////////////

	TEST_METHOD(TestIcloneable)
	{
		auto s = std::make_unique<impl>();
		auto f = s->clone();
		Assert::IsNotNull(s.get());
		Assert::IsNotNull(f.get());
	};
};

} // namespace FrameworkTesting