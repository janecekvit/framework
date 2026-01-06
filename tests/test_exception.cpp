#include "compatibility/compiler_support.h"
#include "exception/exception.h"

#include <gtest/gtest.h>

#if !defined(HAS_STD_FORMAT)
#include <sstream>
#endif

using namespace janecekvit;

namespace framework_tests
{

class test_exception : public ::testing::Test
{
};

std::string GetStringLocation(const std::source_location& location, const std::thread::id& thread, const std::string& suffix)
{
	auto file_name = location.file_name();
	auto function_name = location.function_name();
	auto line = location.line();
	auto column = location.column();

#ifdef __cpp_lib_formatters
	std::string thread_str = std::format("{}", thread);
#else
	std::ostringstream thread_oss;
	thread_oss << thread;
	std::string thread_str = thread_oss.str();
#endif

#if defined(HAS_STD_FORMAT)
	return std::vformat("File: {}({}:{}) '{}'. Thread: {}. {}", std::make_format_args(file_name, line, column, function_name, thread_str, suffix));
#else
	std::ostringstream oss;
	oss << "File: " << file_name << "(" << line << ":" << column << ") '" << function_name << "'. Thread: " << thread_str << ". " << suffix;
	return oss.str();
#endif
}

TEST_F(test_exception, TestSimpleTextException)
{
#if defined(HAS_STD_FORMAT)
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
#else
	try
	{
		throw exception::exception("Ano: true, Ne: false.");
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

	try
	{
		throw exception::exception("Ano: true, Ne: false.", std::source_location::current(), std::this_thread::get_id());
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}
#endif

	try
	{
		throw exception::exception();
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

#if defined(HAS_STD_FORMAT)
	try
	{
		exception::throw_exception("Ano: {}, Ne: {}.", true, false);
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}
#else
	try
	{
		exception::throw_exception("Ano: true, Ne: false.");
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}
#endif

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
#if defined(HAS_STD_FORMAT)
		exception::throw_exception("Ano: {}, Ne: {}.", true, false);
#else
		exception::throw_exception("Ano: true, Ne: false.");
#endif
	}
	catch (const std::exception&)
	{
		caught = true;
	}

	ASSERT_TRUE(caught);
}

TEST_F(test_exception, TestWideTextException)
{
#if defined(HAS_STD_FORMAT)
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
#else
	try
	{
		throw exception::exception(L"Ano: true, Ne: false.");
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

	try
	{
		throw exception::exception(L"Ano: true, Ne: false.", std::source_location::current(), std::this_thread::get_id());
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}
#endif

	try
	{
		throw exception::exception();
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "");
		ASSERT_STREQ(ex.what(), location.c_str());
	}

#if defined(HAS_STD_FORMAT)
	try
	{
		exception::throw_exception(L"Ano: {}, Ne: {}.", true, false);
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}
#else
	try
	{
		exception::throw_exception(L"Ano: true, Ne: false.");
	}
	catch (const exception::exception& ex)
	{
		auto location = GetStringLocation(ex.source_location(), ex.thread_id(), "Ano: true, Ne: false.");
		ASSERT_STREQ(ex.what(), location.c_str());
	}
#endif

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
#if defined(HAS_STD_FORMAT)
		exception::throw_exception(L"Ano: {}, Ne: {}.", true, false);
#else
		exception::throw_exception(L"Ano: true, Ne: false.");
#endif
	}
	catch (const std::exception&)
	{
		caught = true;
	}

	ASSERT_TRUE(caught);
}
} // namespace framework_tests
