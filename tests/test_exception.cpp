#include "exception/exception.h"

#include <gtest/gtest.h>

using namespace janecekvit;

namespace framework_tests
{

class test_exception : public ::testing::Test
{
};

#ifdef __cpp_lib_concepts

std::string GetStringLocation(int origLine, int origColumn, const std::string& suffix, std::source_location location = std::source_location::current(), std::thread::id thread = std::this_thread::get_id())
{
	auto file_name = location.file_name();
    auto function_name = location.function_name();
	return std::vformat("File: {}({}:{}) '{}'. Thread: {}. {}", std::make_format_args(file_name, origLine, origColumn, function_name, thread, suffix));
}

TEST_F(test_exception, TestSimpleTextException)
{
	try
	{
		throw exception::exception("Ano: {}, Ne: {}.", std::make_tuple(true, false));
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(27, 29, "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

	try
	{
		throw exception::exception(std::source_location::current(), std::this_thread::get_id(), "Ano: {}, Ne: {}.", true, false);
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(37, 52, "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

	try
	{
		throw exception::exception();
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(47, 29, "");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

	try
	{
		exception::throw_exception("Ano: {}, Ne: {}.", true, false);
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(57, 29, "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
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
		auto location = GetStringLocation(70, 29, "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

	try
	{
		throw exception::exception(std::source_location::current(), std::this_thread::get_id(), L"Ano: {}, Ne: {}.", true, false);
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(80, 52, "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

	try
	{
		throw exception::exception();
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(90, 29, "");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

	try
	{
		exception::throw_exception(L"Ano: {}, Ne: {}.", true, false);
	}
	catch (const std::exception& ex)
	{
		auto location = GetStringLocation(100, 29, "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}
}
#else
TEST_F(test_exception, TestExceptionNotSupportedConcepts)
{
}
#endif
} // namespace framework_tests
