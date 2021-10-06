#include "stdafx.h"

#include "CppUnitTest.h"
#include "Framework/Extensions/Concurrent.h"
#include "Framework/Extensions/Extensions.h"
#include "Framework/Extensions/GetterSetter.h"
#include "Framework/Extensions/ResourceWrapper.h"

#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <thread>

namespace Microsoft
{
namespace VisualStudio
{
namespace CppUnitTestFramework
{
//TypeDefs
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

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

using namespace std::string_literals;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
//https://docs.microsoft.com/cs-cz/visualstudio/test/microsoft-visualstudio-testtools-cppunittestframework-api-reference?view=vs-2019

class IInterface
{
public:
	virtual ~IInterface() = default;
	virtual int Do()	  = 0;
};

class CInterface : public virtual IInterface
{
public:
	CInterface()		  = default;
	virtual ~CInterface() = default;
	virtual int Do() final override
	{
		return 1111;
	}
};

class IParamTest
{
public:
	virtual ~IParamTest()													= default;
	virtual void Run(_In_ Extensions::Storage::ParameterPackLegacy&& oPack) = 0;
};

class CParamTest : public virtual IParamTest
{
public:
	CParamTest()		  = default;
	virtual ~CParamTest() = default;
	virtual void Run(_In_ Extensions::Storage::ParameterPackLegacy&& oPack) override
	{
		/*int a = 0;
		int b = 0;
		int *c = nullptr;
		std::shared_ptr<int> d = nullptr;*/
		CInterface* pInt = nullptr;

		//Use unpack by output parameters
		oPack.GetPack(a, b, c, d, pInt);
		Assert::AreEqual(a, 25);
		Assert::AreEqual(b, 333);
		Assert::AreEqual(*c, 666);
		Assert::AreEqual(*d, 777);
		Assert::AreEqual(pInt->Do(), 1111);

		//Use unpack by return tuple
		auto [iNumber1, iNumber2, pNumber, pShared, pInterface] = oPack.GetPack<int, int, int*, std::shared_ptr<int>, CInterface*>();

		Assert::AreEqual(iNumber1, 25);
		Assert::AreEqual(iNumber2, 333);
		Assert::AreEqual(*pNumber, 666);
		Assert::AreEqual(*pShared, 777);
		Assert::AreEqual(pInterface->Do(), 1111);
	}

public:
	int a				   = 0;
	int b				   = 0;
	int* c				   = nullptr;
	std::shared_ptr<int> d = nullptr;
};

class IParamTest2
{
public:
	virtual ~IParamTest2()											  = default;
	virtual void Run(_In_ Extensions::Storage::ParameterPack&& oPack) = 0;
};

class CParamTest2 : public virtual IParamTest2
{
public:
	CParamTest2()		   = default;
	virtual ~CParamTest2() = default;
	virtual void Run(_In_ Extensions::Storage::ParameterPack&& oPack) override
	{
		//Use unpack by return tuple
		auto [iNumber1, iNumber2, pNumber, pShared, pInterface] = oPack.GetPack<int, int, int*, std::shared_ptr<int>, CInterface*>();

		Assert::AreEqual(iNumber1, 25);
		Assert::AreEqual(iNumber2, 333);
		Assert::AreEqual(*pNumber, 666);
		Assert::AreEqual(*pShared, 777);
		Assert::AreEqual(pInterface->Do(), 1111);

		a = iNumber1;
		b = iNumber2;
		c = pNumber;
		d = pShared;
	}

public:
	int a				   = 0;
	int b				   = 0;
	int* c				   = nullptr;
	std::shared_ptr<int> d = nullptr;
	std::unique_ptr<int> e = nullptr;
};

void Print(int i, int j, int k, int l, int m)
{
	std::cout << i << j << k << l << m << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////

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
		auto jj = Extensions::ContainerFind(m_mapINT, 5, [](_In_ auto& oResult)
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

namespace TestContainerTraits
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

//different notation
//template <class Other>
//typename std::enable_if_t<Constraints::is_container_v<Other>>
//Test(const Other& other)
//{
//	int i = 0;
//	/*for (const auto& item : other)
//		std::cout << item;*/
//}

template <class Other, typename std::enable_if_t<Constraints::is_container_v<Other>, int> = 0>
auto Test(const Other& other)
{
	int i = 0;
	/*for (const auto& item : other)
		std::cout << item;*/
}

void Test(const int& other)
{
	int i = 0;
	//std::cout << other;
}

void Test(const WithoutEnd& other)
{
	int i = 0;
	//std::cout << other;
}

void Test(const WithoutBegin& other)
{
	int i = 0;
	//std::cout << other;
}

} // namespace TestContainerTraits

namespace TestHeterogeneousContainer
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

		m_pContainer = std::make_unique<Extensions::Storage::HeterogeneousContainer>(fnCallbackInt, fnCallbackInt2);
	}
	~TestHeterogeneousContainer()
	{
	}

	int Call()
	{
		int iCall = 0;
		m_pContainer->CallFirst<std::function<void(int&)>>(iCall);
		return iCall;
	}

private:
	std::unique_ptr<Extensions::Storage::HeterogeneousContainer> m_pContainer = nullptr;
};

} // namespace TestHeterogeneousContainer

class NoGetterSetter
{
public:
	std::vector<int> i;

protected:
	std::vector<int> j;

private:
	std::vector<int> k;
};

class GetterSetterTesting
{
	void Test()
	{
		i->emplace_back(1);
		j->emplace_back(1);
		k->emplace_back(1);
	}

public:
	Extensions::GetterSetter<std::vector<int>> i;

protected:
	Extensions::GetterSetter<std::vector<int>> j;

private:
	Extensions::GetterSetter<std::vector<int>> k;
};

class TestGetterSetter
{
public:
	void AllAccessible()
	{
		{ // int test scope
			Int				 = 5;
			IntSetterPrivate = 6; //   -> private set
			IntBothPrivate	 = 6; //  -> private set and get

			int i1 = Int;
			int i2 = IntSetterPrivate; // -> private set
			int i3 = IntBothPrivate;   // -> private get and set

			int& ilv1 = Int;
			ilv1++;
			int& ilv2 = IntSetterPrivate; //   -> private set
			ilv2++;
			int& ilv3 = IntBothPrivate; //  -> private get
			ilv3++;
		}

		Bool = true; // private set

		{
			auto b1 = Vec.begin();
			auto b2 = VecSetterPrivate.begin();
			auto b3 = VecBothPrivate.begin(); // -> private get

			Vec->emplace_back(5);
			VecSetterPrivate->emplace_back(5); // -> private set
			VecBothPrivate->emplace_back(5);   //-> private set
		}
	}

public:
	Extensions::GetterSetter<int> Int;
	Extensions::GetterSetter<int, TestGetterSetter> IntSetterPrivate;
	Extensions::GetterSetter<int, TestGetterSetter, TestGetterSetter> IntBothPrivate;

	Extensions::GetterSetter<std::vector<int>> Vec;
	Extensions::GetterSetter<std::vector<int>, TestGetterSetter> VecSetterPrivate;
	Extensions::GetterSetter<std::vector<int>, TestGetterSetter, TestGetterSetter> VecBothPrivate;

	Extensions::GetterSetter<bool, TestGetterSetter> Bool;
};

////////////////////////////////////////////////////////////////////////////////////////////

namespace FrameworkUT
{
TEST_CLASS(TestTemplates){
	public:

		TEST_METHOD(TestContainerTraits){
			std::vector<int> vec;
std::list<int> list;
std::set<std::string> set;
std::map<int, std::string> map;
std::unordered_set<int, int> uset;
std::unordered_map<std::string, int> umap;

int a;
std::string b;
TestContainerTraits::WithoutEnd c;
TestContainerTraits::WithoutBegin d;
TestContainerTraits::MyContainer e;

Assert::AreEqual(Constraints::is_container_v<decltype(vec)>, true);
Assert::AreEqual(Constraints::is_container_v<decltype(list)>, true);
Assert::AreEqual(Constraints::is_container_v<decltype(set)>, true);
Assert::AreEqual(Constraints::is_container_v<decltype(map)>, true);
Assert::AreEqual(Constraints::is_container_v<decltype(uset)>, true);
Assert::AreEqual(Constraints::is_container_v<decltype(umap)>, true);
Assert::AreEqual(Constraints::is_container_v<decltype(a)>, false);
Assert::AreEqual(Constraints::is_container_v<decltype(b)>, true);
Assert::AreEqual(Constraints::is_container_v<decltype(c)>, false);
Assert::AreEqual(Constraints::is_container_v<decltype(d)>, false);
Assert::AreEqual(Constraints::is_container_v<decltype(e)>, true);

TestContainerTraits::Test(vec);
TestContainerTraits::Test(list);
TestContainerTraits::Test(set);
TestContainerTraits::Test(map);
TestContainerTraits::Test(uset);
TestContainerTraits::Test(umap);
TestContainerTraits::Test(a);
TestContainerTraits::Test(b);
TestContainerTraits::Test(c);
TestContainerTraits::Test(d);
TestContainerTraits::Test(e);
} // namespace FrameworkUT

TEST_METHOD(TestParameterPackLegacy)
{
	CParamTest oTest;

	int* pInt	 = new int(666);
	auto pShared = std::make_shared<int>(777);
	CInterface oInt;

	// Initialize parameter pack
	auto oPack = Extensions::Storage::ParameterPackLegacy(25, 333, pInt, pShared, &oInt);
	oTest.Run(std::move(oPack));

	Assert::AreEqual(oTest.a, 25);
	Assert::AreEqual(oTest.b, 333);
	Assert::AreEqual(*oTest.c, 666);
	Assert::AreEqual(*oTest.d, 777);
	int is = 5;
}

TEST_METHOD(TestParameterPack)
{
	CParamTest2 oTest;

	int* pInt	 = new int(666);
	auto pShared = std::make_shared<int>(777);
	CInterface oInt;

	// Initialize parameter pack
	auto oPack = Extensions::Storage::ParameterPack(25, 333, pInt, pShared, &oInt);
	oTest.Run(std::move(oPack));

	Assert::AreEqual(oTest.a, 25);
	Assert::AreEqual(oTest.b, 333);
	Assert::AreEqual(*oTest.c, 666);
	Assert::AreEqual(*oTest.d, 777);
	int is = 5;
}

TEST_METHOD(TestHeterogeneousContainer)
{
	using namespace std::string_literals;
	std::function<void(int&)> fnCallbackInt = [this](int& i)
	{
		i += 10;
	};

	std::function<void(int&)> fnCallbackInt2 = [this](int& i)
	{
		i += 20;
	};

	std::function<std::string(std::string &&)> fnCallbackString = [](std::string&& s)
	{
		s += "123";
		return s;
	};

	std::function<std::string(std::string &&)> fnCallbackString2 = [](std::string&& s)
	{
		s += "456";
		return s;
	};

	//Test Get and First methods
	Extensions::Storage::HeterogeneousContainer oContainer(25, 331, 1.1, "string"s, "kase"s, std::make_tuple(25, 333), fnCallbackInt, fnCallbackInt2, fnCallbackString, fnCallbackString2);
	auto oResult	= oContainer.Get<std::string>();
	auto oResultInt = oContainer.Get<int>();
	Assert::AreEqual(*oResultInt.begin(), 25);
	Assert::AreEqual(*++oResultInt.begin(), 331);

	Assert::AreEqual(oContainer.Get<int>(0), 25);
	Assert::AreEqual(oContainer.Get<int>(1), 331);
	Assert::AreEqual(oContainer.First<int>(), 25);

	try
	{
		Assert::AreEqual(oContainer.Get<int>(2), 331);
	}
	catch (const std::exception& ex)
	{
		Assert::AreEqual(ex.what(), "HeterogeneousContainer: Cannot retrieve value on position 2 with specified type: int");
	}

	Assert::AreEqual(oContainer.Get<std::string>(0), "string"s);
	Assert::AreEqual(oContainer.Get<std::string>(1), "kase"s);
	Assert::AreEqual(oContainer.First<std::string>(), "string"s);

	//Test visit methods
	oContainer.Visit<int>([&](int& i)
		{
			i += 100;
		});

	bool bFirst = true;
	oContainer.Visit<int>([&](const int& i)
		{
			Assert::AreEqual(i, bFirst ? 125 : 431);
			bFirst = false;
		});

	auto oResultIntNew = oContainer.Get<int>();
	Assert::AreEqual(*oResultIntNew.begin(), 125);
	Assert::AreEqual(*++oResultIntNew.begin(), 431);

	//Test call methods
	int iCallable = 5;
	oContainer.CallFirst<std::function<void(int&)>>(iCallable);
	Assert::AreEqual(15, iCallable);

	oContainer.Call<std::function<void(int&)>>(1, iCallable);
	Assert::AreEqual(35, iCallable);

	std::string sResult = oContainer.CallFirst<std::function<std::string(std::string &&)>>("Test ");
	Assert::AreEqual("Test 123"s, sResult);

	sResult = oContainer.Call<std::function<std::string(std::string &&)>>(1, "Test ");
	Assert::AreEqual("Test 456"s, sResult);

	try
	{
		oContainer.CallFirst<std::function<void(std::string &&)>>("Test ");
	}
	catch (const std::exception& ex)
	{
		Assert::AreEqual(ex.what(), "HeterogeneousContainer: Cannot retrieve value on position 0 with specified type: class std::function<void __cdecl(class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > &&)>");
	}

	iCallable = 0;
	oContainer.CallAll<std::function<void(int&)>>(iCallable);
	Assert::AreEqual(30, iCallable);

	auto listResults = oContainer.CallAll<std::function<std::string(std::string &&)>>("Test ");
	Assert::AreEqual("Test 123"s, *listResults.begin());
	Assert::AreEqual("Test 456"s, *std::next(listResults.begin(), 1));

	//Test size and reset (1)
	Assert::AreEqual(oContainer.Size<std::function<void(int&)>>(), size_t(2));
	Assert::AreEqual(oContainer.Contains<std::function<void(int&)>>(), true);

	Assert::AreEqual(oContainer.Size<std::function<std::string(std::string &&)>>(), size_t(2));
	Assert::AreEqual(oContainer.Contains<std::function<std::string(std::string &&)>>(), true);

	//Test size and reset (2)
	oContainer.Reset<std::function<void(int&)>>();

	Assert::AreEqual(oContainer.Size<std::function<void(int&)>>(), size_t(0));
	Assert::AreEqual(oContainer.Contains<std::function<void(int&)>>(), false);

	Assert::AreEqual(oContainer.Size<std::function<std::string(std::string &&)>>(), size_t(2));
	Assert::AreEqual(oContainer.Contains<std::function<std::string(std::string &&)>>(), true);

	//Test size and reset (3)
	oContainer.Reset();

	Assert::AreEqual(oContainer.Size<std::function<void(int&)>>(), size_t(0));
	Assert::AreEqual(oContainer.Contains<std::function<void(int&)>>(), false);

	Assert::AreEqual(oContainer.Size<std::function<std::string(std::string &&)>>(), size_t(0));
	Assert::AreEqual(oContainer.Contains<std::function<std::string(std::string &&)>>(), false);

	//Test container in nested class
	TestHeterogeneousContainer::TestHeterogeneousContainer oTestContainer;
	Assert::AreEqual(10, oTestContainer.Call());
}

TEST_METHOD(TestContainerExtensions)
{
	std::unordered_map<CPortsHeader, std::string, CPortsHeaderComparator, CPortsHeaderComparator> my_map;
	my_map.emplace(CPortsHeader(5, 6), "tezko");
	for (auto kv : my_map)
	{
		std::cout << kv.first.uSourcePort << " ";
		std::cout << kv.first.uDestinationPort << " ";
		std::cout << kv.second;
	}

	Extensions::ContainerFind(my_map, CPortsHeader(5, 6), [](_In_ const std::string oResult)
		{
			std::cout << oResult << std::endl;
		});

	auto i = Extensions::ContainerFind(my_map, CPortsHeader(5, 6), [](_In_ const std::string oResult) -> int
		{
			std::cout << oResult << std::endl;
			return 58;
		});

	std::unordered_map<int, std::shared_ptr<std::string>> mapSecond;
	mapSecond.emplace(5, std::make_unique<std::string>("s"));
	auto j = Extensions::ContainerFind(mapSecond, 4, [](_In_ auto& oResult) -> std::shared_ptr<std::string>
		{
			std::cout << oResult;
			return nullptr;
		});

	auto ooo = [](_In_ const auto& oResult) -> std::shared_ptr<std::string>
	{
		std::cout << oResult << std::endl;
		return oResult;
	};

	auto j2 = Extensions::ContainerFind(mapSecond, 5, ooo);

	bool b = {};

	std::map<int, int> mapInts;
	mapInts.emplace(5, 10);
	auto iRes = Extensions::ContainerFind(mapInts, 5, [](_In_ int& oResult)
		{
			oResult += 1;
			return oResult;
		});

	CPortsHeader oVal(10, 20);
	oVal.Compute();

	std::unordered_set<int> setInts{ 5 };
	auto iEx = Extensions::ContainerFind(setInts, 5, [](_In_ const int& oResult) -> int
		{
			return oResult;
		});

	bool bMap = Constraints::is_pair_v<std::iterator_traits<typename std::map<int, int>::iterator>::value_type>;
	bool bVec = Constraints::is_pair_v<std::iterator_traits<typename std::vector<int>::iterator>::value_type>;

	std::vector<int> vecPok = { 1, 4, 6 };
	auto iVecRes			= Extensions::ContainerFind(vecPok, 4, [](_In_ const int& oResult) -> int
		   {
			   return oResult;
		   });

	int isa = 0;
}
TEST_METHOD(TestHashFunction)
{
	std::string s("ano");
	int i		 = 5;
	size_t uHash = Extensions::Hash::Combine(s, i);
	Assert::AreEqual(uHash, static_cast<size_t>(730160148));
}

TEST_METHOD(TestResourceWrapper)
{
	int uLambdaCalled = 0;

	{ // wrapper scope -> Copy constuctor TResource
		auto oWrapperInt = Extensions::ResourceWrapper<int*>(new int(5), [&uLambdaCalled](int*& i)
			{
				delete i;
				i = nullptr;
				uLambdaCalled++;
			});

		int* i = oWrapperInt;
		Assert::AreEqual(*i, 5);
		Assert::AreEqual(*oWrapperInt, 5);

		oWrapperInt = new int(10);
		Assert::AreEqual(*oWrapperInt, 10);
		Assert::AreEqual(uLambdaCalled, 1);
	}

	Assert::AreEqual(uLambdaCalled, 2);

	uLambdaCalled		  = 0;
	auto bCopyDestruction = false;
	{ // wrapper scope -> Copy constuctor Extensions::ResourceWrapper
		auto oWrapperInt = Extensions::ResourceWrapper<int*>(new int(5), [&uLambdaCalled](int*& i)
			{
				delete i;
				i = nullptr;
				uLambdaCalled++;
			});

		int* i = oWrapperInt;
		Assert::AreEqual(*i, 5);
		Assert::AreEqual(*oWrapperInt, 5);

		oWrapperInt = std::move(Extensions::ResourceWrapper<int*>(new int(10), [&uLambdaCalled](int*& i)
			{
				delete i;
				i = nullptr;
				uLambdaCalled++;
			}));
		Assert::AreEqual(*oWrapperInt, 10);
		Assert::AreEqual(uLambdaCalled, 1);

		auto oWrapperInt2 = Extensions::ResourceWrapper<int*>(new int(15), [&uLambdaCalled, &bCopyDestruction](int*& i)
			{
				if (bCopyDestruction)
					return;

				delete i;
				i = nullptr;
				uLambdaCalled++;
				bCopyDestruction = true;
			});

		oWrapperInt = oWrapperInt2;
		Assert::AreEqual(*oWrapperInt, 15);
		Assert::AreEqual(uLambdaCalled, 2);
		Assert::AreEqual(bCopyDestruction, false);
	}
	Assert::AreEqual(uLambdaCalled, 3);
	Assert::AreEqual(bCopyDestruction, true);

	// std namespace test
	auto oWrapperFile = Extensions::ResourceWrapper<std::fstream>(std::fstream("Test", std::ios::binary), [](std::fstream& i)
		{
			i.close();
		});
	bool open		  = oWrapperFile->is_open();

	oWrapperFile.Retrieve([](const std::fstream& fs)
		{
			bool open = fs.is_open();
		});

	Assert::AreEqual(oWrapperFile->is_open(), false);

	{ //Copy constructible
		auto oWrapperInt = Extensions::ResourceWrapper<int*>(new int(5), [](int*& i)
			{
				delete i;
				i = nullptr;
			});

		auto oWrapperUniqueInt = Extensions::ResourceWrapper<std::unique_ptr<int>>(std::make_unique<int>(5), [](std::unique_ptr<int>& i)
			{
				i.release();
			});

		auto oWrapperString = Extensions::ResourceWrapper<std::string>("BLSA", [](std::string& i)
			{
				i.clear();
			});

		auto oWrapperString2 = Extensions::ResourceWrapper<std::string>("DRDF", [](std::string& i)
			{
				i.clear();
			});

		auto oWrapperString3(oWrapperString);
		oWrapperString2		 = oWrapperString;
		auto oWrapperString4 = oWrapperString;

		//auto oWrapperUniqueInt2 = oWrapperUniqueInt; // cannot do that -> unique_ptr has deleted copy constuctor, so wrapper has deleted copy constructor too

		int i = 1;

		// No [[nodiscard]] attribute
		auto it = oWrapperString.begin();
		it		= oWrapperString.end();

		// cannot do that -> int do not contains iterators
		// oWrapperInt.begin();
	}

	int uDestroyed = 0, uDestroyed2 = 0;
	{ //Assigment operator reset
		auto iTest				= new int(5);
		auto oTestOperatorReset = Extensions::ResourceWrapper<int*>(iTest, [&](int*& i)
			{
				if (*i == 5)
					uDestroyed++;

				delete i;
				i = nullptr;
			});

		auto iTest2				 = new int(10);
		auto oTestOperatorReset2 = Extensions::ResourceWrapper<int*>(iTest2, [&](int*& i)
			{
				if (*i == 10)
					uDestroyed2++;

				delete i;
				i = nullptr;
			});

		oTestOperatorReset		 = oTestOperatorReset2;
		auto oTestOperatorReset3 = oTestOperatorReset2;
		Assert::AreEqual(uDestroyed, 1);
		Assert::AreEqual(uDestroyed2, 0);
		Assert::AreEqual(*oTestOperatorReset, 10);
		Assert::AreEqual(*oTestOperatorReset2, 10);
		Assert::AreEqual(*oTestOperatorReset3, 10);
	}

	Assert::AreEqual(uDestroyed, 1);
	Assert::AreEqual(uDestroyed2, 1);

	//Wrapper shared ptr
	{
		auto pNumber		   = std::make_shared<std::string>("A");
		auto oWrapperSharedPtr = Extensions::ResourceWrapper<std::shared_ptr<std::string>>(pNumber, [](std::shared_ptr<std::string>& ptr)
			{
				ptr->append("B");
			});

		oWrapperSharedPtr->append("B");
		Assert::AreEqual(oWrapperSharedPtr->data(), "AB");
	}
}

TEST_METHOD(TestGetterSetterWrapper)
{
	{ // check normal visibility without any getter/setter
		NoGetterSetter o;
		o.i.emplace_back(5);
		Assert::AreEqual(o.i.size(), (size_t) 1);
		// o.j.emplace_back(5); -> protected
		// o.k.emplace_back(5); -> private

		const NoGetterSetter u = o;
		//u.i.emplace_back(5); -> access error, const on modifiable value
		Assert::AreEqual(u.i.size(), (size_t) 1);
		// u.j.emplace_back(5); -> protected
		// u.k.emplace_back(5); -> private
	}

	{ // check getter setter visibility
		GetterSetterTesting x;
		auto ij = x.i->size();
		x.i->emplace_back(5);
		auto z = x.i.begin();
		Assert::AreEqual(x.i.size(), (size_t) 1);

		// check getter setter visibility
		const GetterSetterTesting y = x;
		Assert::AreEqual(y.i.size(), (size_t) 1);
		//y.i->emplace_back(5); ->access error, const on modifiable value
	}

	TestGetterSetter visibility;
	visibility.AllAccessible();

	{ // int test scope
		visibility.Int = 5;
		//visibility.IntSetterPrivate = 6; //   -> private set
		//visibility.IntBothPrivate = 6; //  -> private set and get

		int i1 = visibility.Int;
		int i2 = visibility.IntSetterPrivate; //   -> private set

		//int i3 = visibility.IntBothPrivate; // -> private get and set

		int& ilv1 = visibility.Int;
		// int& ilv2 = visibility.IntSetterPrivate; //   -> private set
		// int& ilv3 = visibility.IntBothPrivate;	 //  -> private get and set
	}

	{ // bool explicit conversions

		int iCondition = 0;
		if (visibility.Bool) // -> non-const explicit bool
			iCondition++;

		if (const auto& constVisivility = visibility; constVisivility.Bool) // -> const explicit bool
			iCondition++;

		if (visibility.Int) // -> non-const explicit bool
			iCondition++;

		if (const auto& constVisivility = visibility; constVisivility.Int) // -> const explicit bool
			iCondition++;

		Assert::AreEqual(iCondition, 4);
	}

	{ //container tests
		auto beg1 = visibility.Vec.begin();
		auto beg2 = visibility.VecSetterPrivate.begin();
		// visibility.VecBothPrivate.begin(); -> private get

		visibility.Vec->emplace_back(5);
		//visibility.VecSetterPrivate->emplace_back(5);  -> private set
		//visibility.VecBothPrivate->emplace_back(5);  -> private set
	}
}

TEST_METHOD(TestTupleExtensions)
{
	auto oHeterogeneousContainer = Extensions::Tuple::Unpack(std::make_tuple(1, 2, 3, "1", "10"));
	Assert::AreEqual(oHeterogeneousContainer.Get<int>(), std::list<int>{ 1, 2, 3 });
	Assert::AreEqual(oHeterogeneousContainer.Get<const char*>(), std::list<const char*>{ "1", "10" });

	auto oStream = Extensions::Tuple::Print(std::make_tuple(1, 2, 3, "1", "10"), std::string(", "));
	Assert::AreEqual(oStream.str(), "1, 2, 3, 1, 10, "s);

	auto fnCallback = [](auto&&... oArgs) -> int
	{
		auto tt = std::forward_as_tuple(oArgs...);
		return std::get<0>(tt);
	};
	auto oResultGenerator = Extensions::Tuple::Generate<10>(fnCallback);
	Assert::AreEqual(oResultGenerator, std::make_tuple(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));
}
}
;
}