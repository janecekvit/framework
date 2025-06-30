#include "extensions/cloneable.h"
#include "extensions/extensions.h"
#include "storage/heterogeneous_container.h"
#include "synchronization/concurrent.h"

#include <future>
#include <gtest/gtest.h>
#include <iostream>
#include <ranges>
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
using namespace std::string_literals;

struct impl : public virtual extensions::cloneable<impl>
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

	Ports(unsigned int uSPort, unsigned int uDPort)
		: uSourcePort(uSPort)
		, uDestinationPort(uDPort)
	{
		m_mapINT.emplace(uSPort, uDPort);
	}

	void Compute() const
	{
		extensions::execute_on_container(m_mapINT, 5, [](auto& oResult)
			{
				return oResult;
			});
	}

private:
	std::unordered_map<int, int> m_mapINT;
};

struct PortsCmp
{
	size_t operator()(const Ports& x) const
	{
		return (std::hash<int>()(x.uSourcePort) ^ std::hash<int>()(x.uDestinationPort));
	}

	bool operator()(const Ports& a, const Ports& b) const
	{
		return a.uSourcePort == b.uSourcePort && a.uDestinationPort == b.uDestinationPort;
	}
};

namespace framework_tests
{
class test_constraints : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}
};

TEST_F(test_constraints, TestContainerTraitsSTL)
{
	std::vector<int> vec;
	std::list<int> list;
	std::set<std::string> set;
	std::map<int, std::string> map;
	std::unordered_set<int, int> uset;
	std::unordered_map<std::string, int> umap;

	ASSERT_EQ(constraints::is_container_v<decltype(vec)>, true);
	ASSERT_EQ(constraints::is_container_v<decltype(list)>, true);
	ASSERT_EQ(constraints::is_container_v<decltype(set)>, true);
	ASSERT_EQ(constraints::is_container_v<decltype(map)>, true);
	ASSERT_EQ(constraints::is_container_v<decltype(uset)>, true);
	ASSERT_EQ(constraints::is_container_v<decltype(umap)>, true);
}

TEST_F(test_constraints, TestContainerTraits)
{
	struct WithoutEnd
	{
		int* begin() { return nullptr; }
		size_t size() { return 0; }
	};

	struct WithoutBegin
	{
		int* end() { return nullptr; }
		size_t size() { return 0; }
	};

	struct WithoutSize
	{
		int* begin() { return nullptr; }
		int* end() { return nullptr; }
	};

	struct MyContainer
	{
		int* begin(){ return nullptr; }
		int* end(){ return nullptr; }
		size_t size(){ return 0; }
	};

	int a;
	std::string b;
	WithoutEnd c;
	WithoutBegin d;
	WithoutSize e;
	MyContainer f;

	ASSERT_EQ(constraints::is_container_v<decltype(a)>, false);
	ASSERT_EQ(constraints::is_container_v<decltype(b)>, true);
	ASSERT_EQ(constraints::is_container_v<decltype(c)>, false);
	ASSERT_EQ(constraints::is_container_v<decltype(d)>, false);
	ASSERT_EQ(constraints::is_container_v<decltype(e)>, false);
	ASSERT_EQ(constraints::is_container_v<decltype(f)>, true);
}

} // namespace framework_tests
