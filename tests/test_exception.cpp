#include "exception/exception.h"

#include <gtest/gtest.h>

using namespace janecekvit;

namespace framework_tests
{
class test_exception : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}
};

#ifdef __cpp_lib_concepts

std::string GetStringLocation(int origLine, int origColumn, std::string suffix, std::source_location location = std::source_location::current(), std::thread::id&& thread = std::this_thread::get_id())
{
	return std::format("File: {}({}:{}) '{}'. Thread: {}. {}", location.file_name(), origLine, origColumn, location.function_name(), thread, suffix);
}

TEST_F(test_exception, TestSimpleTextException)
{
	try
	{
		throw exception::exception("Ano: {}, Ne: {}.", std::make_tuple(true, false));
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(34, 29, "Ano: true, Ne: false.");
		ASSERT_EQ(ex.what(), location);
	}

	try
	{
		throw exception::exception(std::source_location::current(), std::this_thread::get_id(), "Ano: {}, Ne: {}.", true, false);
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(44, 52, "Ano: true, Ne: false.");
		ASSERT_EQ(ex.what(), location);
	}

	try
	{
		throw exception::exception();
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(54, 29, "");
		ASSERT_EQ(ex.what(), location);
	}

	try
	{
		exception::throw_exception("Ano: {}, Ne: {}.", true, false);
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(64, 29, "Ano: true, Ne: false.");
		ASSERT_EQ(ex.what(), location);
	}
}

TEST_F(test_exception, TestWideTextException)
{
	try
	{
		throw exception::exception(L"Ano: {}, Ne: {}.", std::make_tuple(true, false));
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(77, 29, "Ano: true, Ne: false.");
		ASSERT_EQ(ex.what(), location);
	}

	try
	{
		throw exception::exception(std::source_location::current(), std::this_thread::get_id(), L"Ano: {}, Ne: {}.", true, false);
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(87, 52, "Ano: true, Ne: false.");
		ASSERT_EQ(ex.what(), location);
	}

	try
	{
		throw exception::exception();
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(97, 29, "");
		ASSERT_EQ(ex.what(), location);
	}

	try
	{
		exception::throw_exception(L"Ano: {}, Ne: {}.", true, false);
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(107, 29, "Ano: true, Ne: false.");
		ASSERT_EQ(ex.what(), location);
	}
}
#else
TEST_F(test_exception, TestExceptionNotSupportedConcepts)
{
}
#endif
} // namespace framework_tests
