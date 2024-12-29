#include "stdafx.h"

#include "exception/exception.h"

#include <gtest/gtest.h>

using namespace janecekvit;

namespace FrameworkTesting
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

std::string GetStringLocation(int origLine, int origColumn, std::string suffix, std::source_location location = std::source_location::current())
{
	return std::format("File: {}({}:{}) '{}'. {}", location.file_name(), origLine, origColumn, location.function_name(), suffix);
}
TEST_F(test_exception, TestSimpleTextException)
{
	try
	{
		throw exception::exception("Ano: {}, Ne: {}.", std::make_tuple(true, false));
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(33, 29, "Ano: true, Ne: false.");
		ASSERT_EQ(ex.what(), location);
	}

	try
	{
		throw exception::exception(std::source_location::current(), "Ano: {}, Ne: {}.", true, false);
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(43, 52, "Ano: true, Ne: false.");
		ASSERT_EQ(ex.what(), location);
	}

	try
	{
		throw exception::exception();
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(53, 29, "");
		ASSERT_EQ(ex.what(), location);
	}

	try
	{
		exception::throw_exception("Ano: {}, Ne: {}.", true, false);
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(63, 29, "Ano: true, Ne: false.");
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
		auto location = GetStringLocation(76, 29, "Ano: true, Ne: false.");
		ASSERT_EQ(ex.what(), location);
	}

	try
	{
		throw exception::exception(std::source_location::current(), L"Ano: {}, Ne: {}.", true, false);
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(86, 52, "Ano: true, Ne: false.");
		ASSERT_EQ(ex.what(), location);
	}

	try
	{
		throw exception::exception();
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(96, 29, "");
		ASSERT_EQ(ex.what(), location);
	}

	try
	{
		exception::throw_exception(L"Ano: {}, Ne: {}.", true, false);
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(106, 29, "Ano: true, Ne: false.");
		ASSERT_EQ(ex.what(), location);
	}
}
#else
TEST_F(test_exception, TestExceptionNotSupportedConcepts)
{
}
#endif
} // namespace FrameworkTesting
