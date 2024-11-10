#include "stdafx.h"

#include "CppUnitTest.h"
#include "extensions/cloneable.h"
#include "extensions/property.h"
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

using namespace std::string_literals;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
// https://docs.microsoft.com/cs-cz/visualstudio/test/microsoft-visualstudio-testtools-cppunittestframework-api-reference?view=vs-2019

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

class PropertyTesting
{
public:
	extensions::property<std::vector<int>> Public;

protected:
	extensions::property<std::vector<int>> Protected;

private:
	extensions::property<std::vector<int>> Private;
};

struct SimpleValue
{
	SimpleValue(int val)
		: Value(val)
	{
	}

	 std::vector<int> Value;
};

struct PropertyHolder
{
	extensions::property<SimpleValue, PropertyHolder, PropertyHolder> Value;

	PropertyHolder()
		: Value(0)
	{
	}

	PropertyHolder(const SimpleValue& value)
		: Value(value)
	{
	}

	PropertyHolder(SimpleValue&& value)
		: Value(std::move(value))
	{
	}

	
	PropertyHolder(const extensions::property<SimpleValue, PropertyHolder, PropertyHolder>& value)
		: Value(value)
	{
	}

	PropertyHolder(extensions::property<SimpleValue, PropertyHolder, PropertyHolder>&& value)
		: Value(std::move(value))
	{
	}


	PropertyHolder& operator=(const SimpleValue& value)
	{
		Value = value;
		return *this;
	}

	PropertyHolder& operator=(SimpleValue&& value)
	{
		Value = std::move(value);
		return *this;
	}

	bool CheckValue(const SimpleValue& value) const
	{
		return value.Value == Value->Value;
	}
};


////////////////////////////////////////////////////////////////////////////////////////////

namespace FrameworkTesting
{
ONLY_USED_AT_NAMESPACE_SCOPE class test_property : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_property> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
public:
	TEST_METHOD(TestPropertyConstruction)
	{
		SimpleValue intHolder(5);
		const SimpleValue intHolderConst(10);
		SimpleValue intHolderMove(15);

		// construction tests by value
		extensions::property<SimpleValue> value(intHolder);
		Assert::AreEqual(value->Value.size(), size_t(5));

		const extensions::property<SimpleValue> valueConst(intHolderConst);
		Assert::AreEqual(valueConst->Value.size(), size_t(10));

		extensions::property<SimpleValue> valueMove(std::move(intHolderMove));
		Assert::AreEqual(valueMove->Value.size(), size_t(15));

		// construction tests by property
		extensions::property<SimpleValue> valueByProperty(value);
		Assert::AreEqual(valueByProperty->Value.size(), size_t(5));

		extensions::property<SimpleValue> valueByPropertyConst(valueConst);
		Assert::AreEqual(valueByPropertyConst->Value.size(), size_t(10));

		extensions::property<SimpleValue> valueByPropertyMove(std::move(valueMove));
		Assert::AreEqual(valueByPropertyMove->Value.size(), size_t(15));
	}

	TEST_METHOD(TestPropertyConstructionPrivate)
	{
		SimpleValue intHolder(5);
		const SimpleValue intHolderConst(10);
		SimpleValue intHolderMove(15);

		// construction tests by value
		PropertyHolder value(intHolder);
		Assert::IsTrue(value.CheckValue(5));

		PropertyHolder valueConst(intHolderConst);
		Assert::IsTrue(valueConst.CheckValue(10));

		PropertyHolder valueMove(std::move(intHolderMove));
		Assert::IsTrue(valueMove.CheckValue(15));

		// construction tests by property
		PropertyHolder valueByProperty(value);
		Assert::IsTrue(valueByProperty.CheckValue(size_t(5)));

		PropertyHolder valueByPropertyConst(valueConst);
		Assert::IsTrue(valueByPropertyConst.CheckValue(size_t(10)));

		PropertyHolder valueByPropertyMove(std::move(valueMove));
		Assert::IsTrue(valueByPropertyMove.CheckValue(size_t(15)));

		// do not work, private constructor
		/*extensions::property<SimpleValue, PropertyHolder, PropertyHolder> value(intHolder);
		const extensions::property<SimpleValue, PropertyHolder, PropertyHolder> valueConst(intHolderConst);
		extensions::property<SimpleValue, PropertyHolder, PropertyHolder> valueMove(std::move(intHolderMove));*/

	}

	TEST_METHOD(TestPropertyAssign)
	{
		SimpleValue intHolder(5);
		const SimpleValue intHolderConst(10);
		SimpleValue intHolderMove(15);

		auto value = extensions::property<SimpleValue>(intHolder);
		const auto valueConst = extensions::property<SimpleValue>(intHolderConst);
		auto valueMove = extensions::property<SimpleValue>(std::move(intHolderMove));

		// construction tests by property
		auto valueCopy = value;
		Assert::AreEqual(valueCopy->Value.size(), size_t(5));

		const auto valueCopyConst = valueConst;
		Assert::AreEqual(valueCopyConst->Value.size(), size_t(10));

		auto valueCopyMove = std::move(valueMove);
		Assert::AreEqual(valueCopyMove->Value.size(), size_t(15));
		Assert::AreEqual(valueMove->Value.size(), size_t(0));
	}

	TEST_METHOD(TestPropertyAssignPrivate)
	{
		SimpleValue intHolder(5);
		const SimpleValue intHolderConst(10);
		SimpleValue intHolderMove(15);

		PropertyHolder value;
		value = intHolder;
		Assert::IsTrue(value.CheckValue(5));

		PropertyHolder valueConst;
		valueConst = intHolderConst;
		Assert::IsTrue(valueConst.CheckValue(10));

		PropertyHolder valueMove;
		valueMove = std::move(intHolderMove);
		Assert::IsTrue(valueMove.CheckValue(15));
	}

	TEST_METHOD(TestPropertyUserDefinedConversions)
	{
		// bool conversions
		auto valueTrue = extensions::property<int>(5);
		Assert::IsTrue(static_cast<bool>(valueTrue));
		const auto valueFalse = extensions::property<int>(0);
		Assert::IsFalse(static_cast<bool>(valueFalse));

		// user defined conversions
		auto value = extensions::property<int>(10);
		const int intValueConst = value;
		const int& intRefValueConst = value;
		int& intRefValue = value;
		int intMoveValue = std::move(value);

		Assert::AreEqual(intValueConst, 10);
		Assert::AreEqual(intRefValueConst, 10);
		Assert::AreEqual(intRefValue, 10);
		Assert::AreEqual(intMoveValue, 10);
		
	}

	TEST_METHOD(TestPropertyContainersMethods)
	{
		auto value = extensions::property<std::vector<int>>(5);
		Assert::AreEqual(value.size(), size_t(5));
		Assert::AreEqual(*value.begin(), 0);
		Assert::AreEqual(*std::prev(value.end()), 0);
	}

	TEST_METHOD(TestPropertyPointerMethods)
	{
		/*auto ptr	 = extensions::property<std::vector<int>*>(new std::vector<int>(5));
		ptr->push_back(5);

		auto pVector = new std::vector<int>(5);
		pVector->push_back(5)
		
		

		auto value = extensions::property<std::unique_ptr<int>>(5);
		value->reset(new int(10));*/

		//address operators
		
	}

	TEST_METHOD(TestPropertyAddressMethods)
	{
		auto intPtr = std::make_shared<int>(5);
		extensions::property<std::shared_ptr<int>> property(intPtr);
		Assert::IsTrue(*&property == intPtr);

		const auto propertyconst = property;
		Assert::IsTrue(*&propertyconst == intPtr);
	}

	TEST_METHOD(TestPropertyAddressMethodsPrivate)
	{
		auto intPtr = std::make_shared<SimpleValue>(5);

		struct PropertyHolderPtr
		{
			PropertyHolderPtr(const std::shared_ptr<SimpleValue>& value)
				: Value(value)
			{
			}

			extensions::property<std::shared_ptr<SimpleValue>, PropertyHolderPtr, PropertyHolderPtr> Value;

		const std::shared_ptr<SimpleValue>* operator&() const
			{
				return &Value;
			}
		};

		PropertyHolderPtr property(intPtr);

		Assert::IsTrue(*&property == intPtr);

		const auto propertyconst = property;
		Assert::IsTrue(*&propertyconst == intPtr);
	}


};

} // namespace FrameworkTesting