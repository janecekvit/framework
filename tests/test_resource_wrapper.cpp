#include "storage/resource_wrapper.h"

#include <fstream>
#include <gtest/gtest.h>

using namespace janecekvit;
using namespace std::string_literals;

namespace framework_tests
{
class test_resource_wrapper : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}
};

TEST_F(test_resource_wrapper, TestRelease)
{
	int* iValueChecker = nullptr;
	{
		auto oWrapperInt = storage::resource_wrapper<int*>(new int(5), [&](int*& i)
			{
				// check if we obtain same pointer through assignment,
				// if yes reset the value checker, so we knew that lambda is called during destruction
				if (iValueChecker == i)
					iValueChecker = nullptr;

				delete i;
				i = nullptr;
			});

		iValueChecker = oWrapperInt;
		ASSERT_NE(nullptr, iValueChecker);
		ASSERT_EQ(*oWrapperInt, 5);
	}

	ASSERT_EQ(nullptr, iValueChecker);
}

TEST_F(test_resource_wrapper, TestUserConversion)
{
	{
		auto oWrapperInt = storage::resource_wrapper<int*>(new int(5), [](int*& i)
			{
				delete i;
				i = nullptr;
			});

		int& iReference = *oWrapperInt;
		int* iUserDefinedCast = oWrapperInt; // user defined conversion
		bool bTrue = oWrapperInt;			 // user defined conversion bool -> true

		ASSERT_NE(nullptr, iUserDefinedCast);

		ASSERT_EQ(*iUserDefinedCast, 5);
		ASSERT_EQ(*oWrapperInt, 5);
		ASSERT_EQ(iReference, 5);
		ASSERT_TRUE(bTrue);
	}
}

TEST_F(test_resource_wrapper, TestReset)
{
	int* iValueChecker = nullptr;
	{
		auto oWrapperInt = storage::resource_wrapper<int*>(new int(5), [&](int*& i)
			{
				// check if we obtain same pointer through assignment,
				// if yes reset the value checker, so we knew that lambda is called during destruction
				if (iValueChecker == i)
					iValueChecker = nullptr;

				delete i;
				i = nullptr;
			});

		iValueChecker = oWrapperInt;
		ASSERT_NE(nullptr, iValueChecker);

		// release resource before scope ends
		oWrapperInt.reset();

		ASSERT_EQ(nullptr, iValueChecker);
	}

	ASSERT_EQ(nullptr, iValueChecker);
}

TEST_F(test_resource_wrapper, TestRetrieve)
{
	auto oWrapperList = storage::resource_wrapper<std::list<int>>(std::list<int>{ 1, 2 }, [](std::list<int>& i)
		{
			i.clear();
		});

	ASSERT_EQ(size_t{ 2 }, oWrapperList->size());

	bool bRetrieveCalled = false;
	oWrapperList.retrieve([&](auto&& list)
		{
			ASSERT_EQ(size_t{ 2 }, oWrapperList->size());
			bRetrieveCalled = true;
		});

	ASSERT_TRUE(bRetrieveCalled);
}

TEST_F(test_resource_wrapper, TestUpdate)
{
	auto oWrapperList = storage::resource_wrapper<std::list<int>>(std::list<int>{ 1, 2 }, [](std::list<int>& i)
		{
			i.clear();
		});

	ASSERT_EQ(size_t{ 2 }, oWrapperList->size());

	bool bRetrieveCalled = false;
	oWrapperList.update([](auto&& list)
		{
			list.emplace_back(3);
		});

	ASSERT_EQ(size_t{ 3 }, oWrapperList->size());
}

TEST_F(test_resource_wrapper, TestContainers)
{
	{ // Copy constructible
		auto list = std::list<int>{
			10, 20, 30
		};
		auto oWrapperList = storage::resource_wrapper<std::list<int>>(std::move(list), [](std::list<int>& i)
			{
				i.clear();
			});

		ASSERT_EQ(size_t{ 3 }, oWrapperList->size());

		// No [[nodiscard]] attribute
		auto it = oWrapperList.begin();
		auto it2 = --oWrapperList.end();

		ASSERT_EQ(10, *it);
		ASSERT_EQ(30, *it2);

		ASSERT_EQ(size_t{ 3 }, oWrapperList->size());
	}
}

TEST_F(test_resource_wrapper, TestReassignment)
{
	int uDeleterCalled = 0;

	{ // wrapper scope -> copy constructor by assignment operator
		auto oWrapperInt = storage::resource_wrapper<int*>(new int(5), [&uDeleterCalled](int*& i)
			{
				delete i;
				i = nullptr;
				uDeleterCalled++;
			});

		ASSERT_EQ(5, *oWrapperInt);

		// assignment
		oWrapperInt = new int(10);
		ASSERT_EQ(10, *oWrapperInt);
		ASSERT_EQ(1, uDeleterCalled);
	}

	ASSERT_EQ(2, uDeleterCalled);
}

TEST_F(test_resource_wrapper, TestMoveRessignmentWithCopyDestruction)
{
	int uDeleterCalled = 0;
	{ // wrapper scope -> Copy constructor by assignment operator
		auto oWrappedInt1 = storage::resource_wrapper<int*>(new int(5), [&uDeleterCalled](int*& i)
			{
				delete i;
				i = nullptr;
				uDeleterCalled++;
			});

		int* i = oWrappedInt1;
		ASSERT_EQ(*i, 5);
		ASSERT_EQ(*oWrappedInt1, 5);
		ASSERT_EQ(uDeleterCalled, 0);

		// Move re-assignment
		oWrappedInt1 = std::move(storage::resource_wrapper<int*>(new int(10), [&uDeleterCalled](int*& i)
			{
				delete i;
				i = nullptr;
				uDeleterCalled++;
			}));

		ASSERT_EQ(*oWrappedInt1, 10);
		ASSERT_EQ(uDeleterCalled, 1);

		auto oWrappedInt2 = storage::resource_wrapper<int*>(new int(15), [&uDeleterCalled](int*& i)
			{
				delete i;
				i = nullptr;
				uDeleterCalled++;
			});

		oWrappedInt1 = oWrappedInt2;
		ASSERT_EQ(*oWrappedInt1, 15);
		ASSERT_EQ(uDeleterCalled, 2);
	}
	ASSERT_EQ(uDeleterCalled, 3);
}

TEST_F(test_resource_wrapper, TestDeleterException)
{
	auto oWrappedInt = storage::resource_wrapper(new int(5), [](int*& i)
		{
			delete i;
			i = nullptr;
			throw std::runtime_error("Deleter exception");
		});

	try
	{
		ASSERT_EQ(*oWrappedInt, 5);
		oWrappedInt.reset();
	}
	catch (const std::exception&)
	{
		FAIL();
	}
}
#ifdef __cpp_lib_concepts
TEST_F(test_resource_wrapper, TestDeleterExceptionCallback)
{
	bool callbackCalled = false;
	auto oWrappedInt = storage::resource_wrapper(
		new int(5), [](int*& i)
		{
			delete i;
			i = nullptr;
			throw std::runtime_error("Deleter exception");
		},
		[&callbackCalled](const std::exception& ex)
		{
			callbackCalled = true;
			ASSERT_THROW(throw ex, std::exception);
		});

	ASSERT_EQ(*oWrappedInt, 5);
	ASSERT_FALSE(callbackCalled);
	oWrappedInt.reset();
	ASSERT_TRUE(callbackCalled);
}
#endif
} // namespace framework_tests
