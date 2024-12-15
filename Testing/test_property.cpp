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

	SimpleValue* operator->()
	{
		return Value.operator->();
	}

	const SimpleValue* operator->() const
	{
		return Value.operator->();
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
	TEST_METHOD(TestPropertyArrowOperator)
	{
		auto property = extensions::property<std::vector<int>>(std::vector<int>(5));
		property->emplace_back(10);
		Assert::AreEqual(property->size(), size_t(6));

		const auto propertyConst = property;
		Assert::AreEqual(propertyConst->at(5), 10);
	}
	
	TEST_METHOD(TestPropertyArrowOperatorPrivate)
	{
		SimpleValue intHolder(5);
		PropertyHolder property(intHolder);

		property->Value.emplace_back(10);
		Assert::AreEqual(property->Value.size(), size_t(6));

		const auto propertyConst = property;
		Assert::AreEqual(propertyConst->Value.at(5), 10);
	}

	TEST_METHOD(TestPropertyArrowOperatorOnPointer)
	{
		auto vector = new std::vector<int>(5);
		auto clean	= extensions::finally([&]
			 {
				 delete vector;
			 });

		auto property = extensions::property<std::vector<int>*>(vector);
		property->emplace_back(10);
		Assert::AreEqual(property->size(), size_t(6));

		const auto propertyConst = property;
		Assert::AreEqual(propertyConst->at(5), 10);
	}

	TEST_METHOD(TestPropertyArrowOperatorOnPointerPrivate)
	{
		struct PropertyHolderPtr
		{
			PropertyHolderPtr(std::vector<int>* value)
				: Value(value)
			{
			}

			std::vector<int>* operator->()
			{
				return Value.operator->();
			}

			const std::vector<int>* operator->() const
			{
				return Value.operator->();
			}

			extensions::property<std::vector<int>*, PropertyHolderPtr, PropertyHolderPtr> Value;
		};

		auto vector = new std::vector<int>(5);
		auto clean	= extensions::finally([&]
			 {
				 delete vector;
			 });

		auto property = PropertyHolderPtr(vector);
		property->emplace_back(10);
		Assert::AreEqual(property->size(), size_t(6));

		const auto propertyConst = property;
		Assert::AreEqual(propertyConst->at(5), 10);
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

	TEST_METHOD(TestPropertyWithLambda)
	{
		struct PropertyHolderLambda
		{
			extensions::property<int> Value;

			PropertyHolderLambda()
				: Value([&]() -> const int&
					  {
						  return hiddenValue;
					  },
					  [&](int newValue)
					  {
						  hiddenValue = newValue * newValue;
					  })
			{
			}

			int hiddenValue = 10;
		};

		PropertyHolderLambda property;

		Assert::AreEqual((int) property.Value, 10);
		Assert::AreEqual((int) property.hiddenValue, 10);

		property.Value = 10;
		Assert::AreEqual((int) property.Value, 100);
		Assert::AreEqual((int) property.hiddenValue, 100);
	}

	TEST_METHOD(TestPropertyWithLambdaPrivate)
	{
		struct PropertyHolderLambda
		{
			extensions::property<int, PropertyHolderLambda, PropertyHolderLambda> Value;

			PropertyHolderLambda()
				: Value([&]() -> const int&
					  {
						  return hiddenValue;
					  },
					  [&](int newValue)
					  {
						  hiddenValue = newValue * newValue;
					  })
			{
			}

			int GetValue() const
			{
				return Value;
			}

			void SetValue(int newValue)
			{
				Value = newValue;
			}

			int hiddenValue = 10;
		};

		PropertyHolderLambda property;
		
		Assert::AreEqual(property.GetValue(), 10);
		Assert::AreEqual((int) property.hiddenValue, 10);

		property.SetValue(10);
		Assert::AreEqual(property.GetValue(), 100);
		Assert::AreEqual((int) property.hiddenValue, 100);
	}

	TEST_METHOD(TestPropertyWithLambdaUserDefinedConversions)
	{
		std::vector<int> hiddenValue(10);
		extensions::property<std::vector<int>> property([&]() -> const std::vector<int>&
			{
				return hiddenValue;
			},
			[&](const std::vector<int>& newValue)
			{
				hiddenValue = newValue;
				hiddenValue.emplace_back(1);
			});

		Assert::AreEqual((int) property.size(), 10);
		Assert::AreEqual((int) hiddenValue.size(), 10);

		std::vector<int>& vec = property;
		vec.emplace_back(1);

		Assert::AreEqual((int) property.size(), 11);
		Assert::AreEqual((int) hiddenValue.size(), 11);
		
		std::vector<int> vecMove = std::move(property);
		Assert::AreEqual((int) vecMove.size(), 11);
		Assert::AreEqual((int) hiddenValue.size(), 0);
	}
};

} // namespace FrameworkTesting