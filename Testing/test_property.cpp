#include "stdafx.h"

#include "extensions/cloneable.h"
#include "extensions/property.h"
#include "storage/heterogeneous_container.h"
#include "synchronization/concurrent.h"

#include <future>
#include <gtest/gtest.h>
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

namespace framework_tests
{
class test_property : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}
};

TEST_F(test_property, TestPropertyConstruction)
{
	SimpleValue intHolder(5);
	const SimpleValue intHolderConst(10);
	SimpleValue intHolderMove(15);

	// construction tests by value
	extensions::property<SimpleValue> value(intHolder);
	ASSERT_EQ(value->Value.size(), size_t(5));

	const extensions::property<SimpleValue> valueConst(intHolderConst);
	ASSERT_EQ(valueConst->Value.size(), size_t(10));

	extensions::property<SimpleValue> valueMove(std::move(intHolderMove));
	ASSERT_EQ(valueMove->Value.size(), size_t(15));

	// construction tests by property
	extensions::property<SimpleValue> valueByProperty(value);
	ASSERT_EQ(valueByProperty->Value.size(), size_t(5));

	extensions::property<SimpleValue> valueByPropertyConst(valueConst);
	ASSERT_EQ(valueByPropertyConst->Value.size(), size_t(10));

	extensions::property<SimpleValue> valueByPropertyMove(std::move(valueMove));
	ASSERT_EQ(valueByPropertyMove->Value.size(), size_t(15));
}

TEST_F(test_property, TestPropertyConstructionPrivate)
{
	SimpleValue intHolder(5);
	const SimpleValue intHolderConst(10);
	SimpleValue intHolderMove(15);

	// construction tests by value
	PropertyHolder value(intHolder);
	ASSERT_TRUE(value.CheckValue(5));

	PropertyHolder valueConst(intHolderConst);
	ASSERT_TRUE(valueConst.CheckValue(10));

	PropertyHolder valueMove(std::move(intHolderMove));
	ASSERT_TRUE(valueMove.CheckValue(15));

	// construction tests by property
	PropertyHolder valueByProperty(value);
	ASSERT_TRUE(valueByProperty.CheckValue(size_t(5)));

	PropertyHolder valueByPropertyConst(valueConst);
	ASSERT_TRUE(valueByPropertyConst.CheckValue(size_t(10)));

	PropertyHolder valueByPropertyMove(std::move(valueMove));
	ASSERT_TRUE(valueByPropertyMove.CheckValue(size_t(15)));
}

TEST_F(test_property, TestPropertyAssign)
{
	SimpleValue intHolder(5);
	const SimpleValue intHolderConst(10);
	SimpleValue intHolderMove(15);

	auto value = extensions::property<SimpleValue>(intHolder);
	const auto valueConst = extensions::property<SimpleValue>(intHolderConst);
	auto valueMove = extensions::property<SimpleValue>(std::move(intHolderMove));

	// construction tests by property
	auto valueCopy = value;
	ASSERT_EQ(valueCopy->Value.size(), size_t(5));

	const auto valueCopyConst = valueConst;
	ASSERT_EQ(valueCopyConst->Value.size(), size_t(10));

	auto valueCopyMove = std::move(valueMove);
	ASSERT_EQ(valueCopyMove->Value.size(), size_t(15));
	ASSERT_EQ(valueMove->Value.size(), size_t(0));
}

TEST_F(test_property, TestPropertyAssignPrivate)
{
	SimpleValue intHolder(5);
	const SimpleValue intHolderConst(10);
	SimpleValue intHolderMove(15);

	PropertyHolder value;
	value = intHolder;
	ASSERT_TRUE(value.CheckValue(5));

	PropertyHolder valueConst;
	valueConst = intHolderConst;
	ASSERT_TRUE(valueConst.CheckValue(10));

	PropertyHolder valueMove;
	valueMove = std::move(intHolderMove);
	ASSERT_TRUE(valueMove.CheckValue(15));
}

TEST_F(test_property, TestPropertyUserDefinedConversions)
{
	// bool conversions
	auto valueTrue = extensions::property<int>(5);
	ASSERT_TRUE(static_cast<bool>(valueTrue));
	const auto valueFalse = extensions::property<int>(0);
	ASSERT_FALSE(static_cast<bool>(valueFalse));

	// user defined conversions
	auto value = extensions::property<int>(10);
	const int intValueConst = value;
	const int& intRefValueConst = value;
	int& intRefValue = value;
	int intMoveValue = std::move(value);

	ASSERT_EQ(intValueConst, 10);
	ASSERT_EQ(intRefValueConst, 10);
	ASSERT_EQ(intRefValue, 10);
	ASSERT_EQ(intMoveValue, 10);
}

TEST_F(test_property, TestPropertyContainersMethods)
{
	auto value = extensions::property<std::vector<int>>(5);
	ASSERT_EQ(value.size(), size_t(5));
	ASSERT_EQ(*value.begin(), 0);
	ASSERT_EQ(*std::prev(value.end()), 0);
}

TEST_F(test_property, TestPropertyArrowOperator)
{
	auto property = extensions::property<std::vector<int>>(std::vector<int>(5));
	property->emplace_back(10);
	ASSERT_EQ(property->size(), size_t(6));

	const auto propertyConst = property;
	ASSERT_EQ(propertyConst->at(5), 10);
}

TEST_F(test_property, TestPropertyArrowOperatorPrivate)
{
	SimpleValue intHolder(5);
	PropertyHolder property(intHolder);

	property->Value.emplace_back(10);
	ASSERT_EQ(property->Value.size(), size_t(6));

	const auto propertyConst = property;
	ASSERT_EQ(propertyConst->Value.at(5), 10);
}

TEST_F(test_property, TestPropertyArrowOperatorOnPointer)
{
	auto vector = new std::vector<int>(5);
	auto clean = extensions::finally([&]
		{
			delete vector;
		});

	auto property = extensions::property<std::vector<int>*>(vector);
	property->emplace_back(10);
	ASSERT_EQ(property->size(), size_t(6));

	const auto propertyConst = property;
	ASSERT_EQ(propertyConst->at(5), 10);
}

TEST_F(test_property, TestPropertyArrowOperatorOnPointerPrivate)
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
	auto clean = extensions::finally([&]
		{
			delete vector;
		});

	auto property = PropertyHolderPtr(vector);
	property->emplace_back(10);
	ASSERT_EQ(property->size(), size_t(6));

	const auto propertyConst = property;
	ASSERT_EQ(propertyConst->at(5), 10);
}

TEST_F(test_property, TestPropertyAddressMethods)
{
	auto intPtr = std::make_shared<int>(5);
	extensions::property<std::shared_ptr<int>> property(intPtr);
	ASSERT_TRUE(*&property == intPtr);

	const auto propertyconst = property;
	ASSERT_TRUE(*&propertyconst == intPtr);
}

TEST_F(test_property, TestPropertyAddressMethodsPrivate)
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
	ASSERT_TRUE(*&property == intPtr);

	const auto propertyconst = property;
	ASSERT_TRUE(*&propertyconst == intPtr);
}

TEST_F(test_property, TestPropertyWithLambda)
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

	ASSERT_EQ((int) property.Value, 10);
	ASSERT_EQ((int) property.hiddenValue, 10);

	property.Value = 10;
	ASSERT_EQ((int) property.Value, 100);
	ASSERT_EQ((int) property.hiddenValue, 100);
}

TEST_F(test_property, TestPropertyWithLambdaPrivate)
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

	ASSERT_EQ(property.GetValue(), 10);
	ASSERT_EQ((int) property.hiddenValue, 10);

	property.SetValue(10);
	ASSERT_EQ(property.GetValue(), 100);
	ASSERT_EQ((int) property.hiddenValue, 100);
}

TEST_F(test_property, TestPropertyWithLambdaUserDefinedConversions)
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

	ASSERT_EQ((int) property.size(), 10);
	ASSERT_EQ((int) hiddenValue.size(), 10);

	std::vector<int>& vec = property;
	vec.emplace_back(1);

	ASSERT_EQ((int) property.size(), 11);
	ASSERT_EQ((int) hiddenValue.size(), 11);

	std::vector<int> vecMove = std::move(property);
	ASSERT_EQ((int) vecMove.size(), 11);
	ASSERT_EQ((int) hiddenValue.size(), 0);
}

} // namespace framework_tests
