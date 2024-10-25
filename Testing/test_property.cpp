#include "stdafx.h"

#include "CppUnitTest.h"
#include "extensions/property.h"
#include "storage/heterogeneous_container.h"
#include "synchronization/concurrent.h"
#include "extensions/cloneable.h"

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


using namespace std::string_literals;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
// https://docs.microsoft.com/cs-cz/visualstudio/test/microsoft-visualstudio-testtools-cppunittestframework-api-reference?view=vs-2019

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
	extensions::property<std::vector<int>> i;

protected:
	extensions::property<std::vector<int>> j;

private:
	extensions::property<std::vector<int>> k;
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
	extensions::property<int> Int;
	extensions::property<int, TestGetterSetter> IntSetterPrivate;
	extensions::property<int, TestGetterSetter, TestGetterSetter> IntBothPrivate;

	extensions::property<std::vector<int>> Vec;
	extensions::property<std::vector<int>, TestGetterSetter> VecSetterPrivate;
	extensions::property<std::vector<int>, TestGetterSetter, TestGetterSetter> VecBothPrivate;

	extensions::property<bool, TestGetterSetter> Bool;
};

////////////////////////////////////////////////////////////////////////////////////////////

namespace FrameworkTesting
{
ONLY_USED_AT_NAMESPACE_SCOPE class test_property : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_property> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
public:

	TEST_METHOD(TestGetterSetterWrapper)
	{
		{ // check normal visibility without any getter/setter
			NoGetterSetter o;
			o.i.emplace_back(5);
			Assert::AreEqual(o.i.size(), (size_t) 1);
			// o.j.emplace_back(5); -> protected
			// o.k.emplace_back(5); -> private

			const NoGetterSetter u = o;
			// u.i.emplace_back(5); -> access error, const on modifiable value
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
			// y.i->emplace_back(5); ->access error, const on modifiable value
		}

		TestGetterSetter visibility;
		visibility.AllAccessible();

		{ // int test scope
			visibility.Int = 5;
			// visibility.IntSetterPrivate = 6; //   -> private set
			// visibility.IntBothPrivate = 6; //  -> private set and get

			int i1 = visibility.Int;
			int i2 = visibility.IntSetterPrivate; //   -> private set

			// int i3 = visibility.IntBothPrivate; // -> private get and set

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

		{ // container tests
			auto beg1 = visibility.Vec.begin();
			auto beg2 = visibility.VecSetterPrivate.begin();
			// visibility.VecBothPrivate.begin(); -> private get

			visibility.Vec->emplace_back(5);
			// visibility.VecSetterPrivate->emplace_back(5);  -> private set
			// visibility.VecBothPrivate->emplace_back(5);  -> private set
		}
	}
};

} // namespace FrameworkTesting