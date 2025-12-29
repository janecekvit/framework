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


TEST_F(test_resource_wrapper, TestDeduction_LValue)
{
	int* ptr = new int(42);
	auto wrapper = storage::resource_wrapper(ptr, [](int*& p)
		{
			delete p;
			p = nullptr;
		});

	ASSERT_EQ(*wrapper, 42);
}

TEST_F(test_resource_wrapper, TestDeduction_ConstLvalue)
{
	const int value = 100;
	auto wrapper = storage::resource_wrapper(value, [](int& v)
		{
			v = 0;
		});

	ASSERT_EQ(static_cast<int>(wrapper), 100);
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
	oWrapperList.retrieve([&](auto&&)
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

		int* result = oWrappedInt1;
		ASSERT_EQ(*result, 5);
		ASSERT_EQ(*oWrappedInt1, 5);
		ASSERT_EQ(uDeleterCalled, 0);

		// Move re-assignment
		oWrappedInt1 = storage::resource_wrapper<int*>(new int(10), [&uDeleterCalled](int*& i)
			{
				delete i;
				i = nullptr;
				uDeleterCalled++;
			});

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


TEST_F(test_resource_wrapper, TestDeleterExceptionCallbackLvalue)
{
	int* ptr = new int(50);
	bool exceptionHandled = false;

	{
		auto wrapper = storage::resource_wrapper(
			ptr,
			[](int*& p)
			{
				delete p;
				p = nullptr;
				throw std::runtime_error("Test exception");
			},
			[&exceptionHandled](const std::exception&)
			{
				exceptionHandled = true;
			});

		ASSERT_EQ(*wrapper, 50);
		wrapper.reset();
	}

	ASSERT_TRUE(exceptionHandled);
}



TEST_F(test_resource_wrapper, TestSimpleStruct)
{
	struct SimpleStruct
	{
		int* value = nullptr;
	};

	int* valueChecker = new int(5);
	SimpleStruct structValue = { valueChecker };

	{
		auto oWrapperStruct = storage::resource_wrapper<SimpleStruct>(structValue, [&valueChecker](SimpleStruct& s)
			{
				if (valueChecker == s.value)
					valueChecker = nullptr;
				delete s.value;
				s.value = nullptr;
			});
		ASSERT_NE(nullptr, valueChecker);
		ASSERT_EQ(*(oWrapperStruct->value), 5);

	}

	ASSERT_EQ(nullptr, valueChecker);
}


TEST_F(test_resource_wrapper, TestNonCopyableResource)
{
	struct NonCopyable
	{
		int value;
		NonCopyable(int v) : value(v) {}
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable(NonCopyable&&) = default;
	};

	bool deleterCalled = false;
	{
		auto wrapper = storage::resource_wrapper<NonCopyable>(
			NonCopyable{ 77 },
			[&deleterCalled](NonCopyable& nc)
			{
				deleterCalled = true;
				nc.value = 0;
			});

		ASSERT_EQ(wrapper->value, 77);
	}
	ASSERT_TRUE(deleterCalled);
}



TEST_F(test_resource_wrapper, TestPerfectForwardingPreservation)
{
	struct TrackableCopyMove
	{
		int* copyCount;
		int* moveCount;

		TrackableCopyMove(int* cc, int* mc) : copyCount(cc), moveCount(mc) {}

		TrackableCopyMove(const TrackableCopyMove& other)
			: copyCount(other.copyCount), moveCount(other.moveCount)
		{
			(*copyCount)++;
		}

		TrackableCopyMove(TrackableCopyMove&& other) noexcept
			: copyCount(other.copyCount), moveCount(other.moveCount)
		{
			(*moveCount)++;
		}
	};

	int copyCount = 0;
	int moveCount = 0;

	{
		// Rvalue → should use move
		auto wrapper1 = storage::resource_wrapper<TrackableCopyMove>(
			TrackableCopyMove{ &copyCount, &moveCount },
			[](TrackableCopyMove&) {});

		ASSERT_EQ(copyCount, 0);
		ASSERT_GE(moveCount, 1);
	}

	copyCount = 0;
	moveCount = 0;

	{
		// Lvalue → should use copy
		TrackableCopyMove obj{ &copyCount, &moveCount };
		auto wrapper2 = storage::resource_wrapper<TrackableCopyMove>(
			obj,
			[](TrackableCopyMove&) {});

		ASSERT_GE(copyCount, 1);
	}
}

TEST_F(test_resource_wrapper, TestFileHandle)
{
	const char* filename = "test_wrapper_file.txt";
	bool fileWasClosed = false;

	{
		auto fileWrapper = storage::resource_wrapper<std::ofstream>(
			std::ofstream(filename),
			[&fileWasClosed](std::ofstream& file)
			{
				if (file.is_open())
				{
					file.close();
					fileWasClosed = true;
				}
			});

		ASSERT_TRUE(fileWrapper->is_open());
		fileWrapper->write("Test data\n", 10);
	}

	ASSERT_TRUE(fileWasClosed);
	std::remove(filename);
}
} // namespace framework_tests
