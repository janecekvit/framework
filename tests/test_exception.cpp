#include "exception/exception.h"

#include <gtest/gtest.h>

using namespace janecekvit;

namespace framework_tests
{

class test_exception : public ::testing::Test
{
};

#ifdef __cpp_lib_concepts

std::string GetStringLocation(const std::source_location& location, const std::thread::id& thread, const std::string& suffix)
{
	auto file_name = location.file_name();
    auto function_name = location.function_name();
	auto line = location.line();
    auto column = location.column();

#ifdef __cpp_lib_formatters
	std::string thread_str = std::format("{}", thread);
#else
	std::ostringstream oss;
	oss << thread ;
	std::string thread_str = oss.str();
#endif

	return std::vformat("File: {}({}:{}) '{}'. Thread: {}. {}", std::make_format_args(file_name, line, column, function_name, thread_str, suffix));

}

TEST_F(test_exception, TestSimpleTextException)
{
	try
	{
		throw exception::exception("Ano: {}, Ne: {}.", std::make_tuple(true, false));
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

	try
	{
		throw exception::exception(std::source_location::current(), std::this_thread::get_id(), "Ano: {}, Ne: {}.", true, false);
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

	try
	{
		throw exception::exception();
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

	try
	{
		exception::throw_exception("Ano: {}, Ne: {}.", true, false);
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

	// std exception catch test
	bool caught = false;
	try
	{
		throw exception::exception();
	}
	catch (const std::exception&)
	{
		caught = true;
	}
	
	ASSERT_TRUE(caught);

	caught = false;
	try
	{
		exception::throw_exception("Ano: {}, Ne: {}.", true, false);
	}
	catch (const std::exception&)
	{
		caught = true;
	}
	
	ASSERT_TRUE(caught);
}

TEST_F(test_exception, TestWideTextException)
{
	try
	{
		throw exception::exception(L"Ano: {}, Ne: {}.", std::make_tuple(true, false));
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

	try
	{
		throw exception::exception(std::source_location::current(), std::this_thread::get_id(), L"Ano: {}, Ne: {}.", true, false);
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

	try
	{
		throw exception::exception();
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

	try
	{
		exception::throw_exception(L"Ano: {}, Ne: {}.", true, false);
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

	// std exception catch test
	bool caught = false;
	try
	{
		throw exception::exception();
	}
	catch (const std::exception&)
	{
		caught = true;
	}
	
	ASSERT_TRUE(caught);

	caught = false;
	try
	{
		exception::throw_exception(L"Ano: {}, Ne: {}.", true, false);
	}
	catch (const std::exception&)
	{
		caught = true;
	}

	ASSERT_TRUE(caught);
}
#else
TEST_F(test_exception, TestExceptionNotSupportedConcepts)
{
}
#endif
} // namespace framework_tests
