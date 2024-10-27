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
ONLY_USED_AT_NAMESPACE_SCOPE class test_constraints : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_constraints> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
public:
	TEST_METHOD(TestContainerTraitsSTL)
	{
		std::vector<int> vec;
		std::list<int> list;
		std::set<std::string> set;
		std::map<int, std::string> map;
		std::unordered_set<int, int> uset;
		std::unordered_map<std::string, int> umap;

		Assert::AreEqual(constraints::is_container_v<decltype(vec)>, true);
		Assert::AreEqual(constraints::is_container_v<decltype(list)>, true);
		Assert::AreEqual(constraints::is_container_v<decltype(set)>, true);
		Assert::AreEqual(constraints::is_container_v<decltype(map)>, true);
		Assert::AreEqual(constraints::is_container_v<decltype(uset)>, true);
		Assert::AreEqual(constraints::is_container_v<decltype(umap)>, true);
	}
	TEST_METHOD(TestContainerTraits)
	{
		struct WithoutEnd
		{
			int* begin();
			size_t size();
		};

		struct WithoutBegin
		{
			int* end();
			size_t size();
		};

		struct WithoutSize
		{
			int* begin();
			int* end();
		};
		{
			int* end();
			size_t size();
		};

		struct MyContainer
		{
			int* begin();
			int* end();
			size_t size();
		};

		int a;
		std::string b;
		WithoutEnd c;
		WithoutBegin d;
		WithoutSize e;
		MyContainer f;

		Assert::AreEqual(constraints::is_container_v<decltype(a)>, false);
		Assert::AreEqual(constraints::is_container_v<decltype(b)>, true);
		Assert::AreEqual(constraints::is_container_v<decltype(c)>, false);
		Assert::AreEqual(constraints::is_container_v<decltype(d)>, false);
		Assert::AreEqual(constraints::is_container_v<decltype(e)>, false);
		Assert::AreEqual(constraints::is_container_v<decltype(f)>, true);
	}
};

} // namespace FrameworkTesting