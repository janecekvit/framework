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

class CPortsHeader
{
public:
	unsigned int uSourcePort, uDestinationPort;

	CPortsHeader(_In_ unsigned int uSPort, _In_ unsigned int uDPort)
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

struct CPortsHeaderComparator
{
	size_t operator()(_In_ const CPortsHeader& x) const
	{
		return (std::hash<int>()(x.uSourcePort) ^ std::hash<int>()(x.uDestinationPort));
	}

	bool operator()(_In_ const CPortsHeader& a, _In_ const CPortsHeader& b) const
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
	TEST_METHOD(TestContainerTraits)
	{
		struct WithoutEnd
		{
			int begin()
			{
				return 1;
			}
		};

		struct WithoutBegin
		{
			int begin()
			{
				return 1;
			}
		};

		struct MyContainer
		{
			int* begin()
			{
				return nullptr;
			}
			int* end()
			{
				return nullptr;
			}
			size_t size()
			{
				return 0;
			}
		};

		std::vector<int> vec;
		std::list<int> list;
		std::set<std::string> set;
		std::map<int, std::string> map;
		std::unordered_set<int, int> uset;
		std::unordered_map<std::string, int> umap;

		int a;
		std::string b;
		WithoutEnd c;
		WithoutBegin d;
		MyContainer e;

		Assert::AreEqual(constraints::is_container_v<decltype(vec)>, true);
		Assert::AreEqual(constraints::is_container_v<decltype(list)>, true);
		Assert::AreEqual(constraints::is_container_v<decltype(set)>, true);
		Assert::AreEqual(constraints::is_container_v<decltype(map)>, true);
		Assert::AreEqual(constraints::is_container_v<decltype(uset)>, true);
		Assert::AreEqual(constraints::is_container_v<decltype(umap)>, true);
		Assert::AreEqual(constraints::is_container_v<decltype(a)>, false);
		Assert::AreEqual(constraints::is_container_v<decltype(b)>, true);
		Assert::AreEqual(constraints::is_container_v<decltype(c)>, false);
		Assert::AreEqual(constraints::is_container_v<decltype(d)>, false);
		Assert::AreEqual(constraints::is_container_v<decltype(e)>, true);

	} // namespace FrameworkTesting

	TEST_METHOD(TestExecuteOnContainer)
	{
		std::unordered_map<CPortsHeader, std::string, CPortsHeaderComparator, CPortsHeaderComparator> my_map;
		my_map.emplace(CPortsHeader(5, 6), "tezko");


		extensions::execute_on_container(my_map, CPortsHeader(5, 6), [](_In_ const std::string oResult)
			{
				std::cout << oResult << std::endl;
			});

		auto i = extensions::execute_on_container(my_map, CPortsHeader(5, 6), [](_In_ const std::string oResult) -> int
			{
				std::cout << oResult << std::endl;
				return 58;
			});

		std::unordered_map<int, std::shared_ptr<std::string>> mapSecond;
		mapSecond.emplace(5, std::make_unique<std::string>("s"));
		auto j = extensions::execute_on_container(mapSecond, 4, [](_In_ auto& oResult) -> std::shared_ptr<std::string>
			{
				std::cout << oResult;
				return nullptr;
			});

		auto ooo = [](_In_ const auto& oResult) -> std::shared_ptr<std::string>
		{
			std::cout << oResult << std::endl;
			return oResult;
		};

		auto j2 = extensions::execute_on_container(mapSecond, 5, ooo);

		bool b = {};

		std::map<int, int> mapInts;
		mapInts.emplace(5, 10);
		auto iRes = extensions::execute_on_container(mapInts, 5, [](_In_ int& oResult)
			{
				oResult += 1;
				return oResult;
			});

		CPortsHeader oVal(10, 20);
		oVal.Compute();

		std::unordered_set<int> setInts{ 5 };
		auto iEx = extensions::execute_on_container(setInts, 5, [](_In_ const int& oResult) -> int
			{
				return oResult;
			});

		bool bMap = constraints::is_pair_v<std::iterator_traits<typename std::map<int, int>::iterator>::value_type>;
		bool bVec = constraints::is_pair_v<std::iterator_traits<typename std::vector<int>::iterator>::value_type>;

		std::vector<int> vecPok = { 1, 4, 6 };
		auto iVecRes			= extensions::execute_on_container(vecPok, 4, [](_In_ const int& oResult) -> int
					   {
				   return oResult;
			   });

		int isa = 0;
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