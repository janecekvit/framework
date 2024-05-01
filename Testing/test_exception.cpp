#include "stdafx.h"

#include "CppUnitTest.h"
#include "exception/exception.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace janecekvit;

namespace FrameworkTesting
{
ONLY_USED_AT_NAMESPACE_SCOPE class test_exception : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_exception> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
public:
#if __cplusplus >= __cpp_lib_concepts

	std::string GetStringLocation(int origLine, int origColumn, std::string suffix, std::source_location location = std::source_location::current())
	{
		return std::format("File: {}({}:{}) '{}'. {}", location.file_name(), origLine, origColumn, location.function_name(), suffix);
	}
	TEST_METHOD(TestSimpleTextException)
	{
		try
		{
			throw exception::exception("Ano: {}, Ne: {}.", std::make_tuple(true, false));
		}
		catch (const exception::exception& ex)
		{
			auto location = GetStringLocation(24, 30, "Ano: true, Ne: false.");
			Assert::AreEqual(ex.what(), location.data());
		}

		try
		{
			throw exception::exception(std::source_location::current(), "Ano: {}, Ne: {}.", true, false);
		}
		catch (const std::exception& ex)
		{
			auto location = GetStringLocation(34, 53, "Ano: true, Ne: false.");
			Assert::AreEqual(ex.what(), location.data());
		}

		try
		{
			throw exception::exception();
		}
		catch (const std::exception& ex)
		{
			auto location = GetStringLocation(44, 30, "");
			Assert::AreEqual(ex.what(), location.data());
		}

		try
		{
			exception::throw_exception("Ano: {}, Ne: {}.", true, false);
		}
		catch (const std::exception& ex)
		{
			auto location = GetStringLocation(54, 30, "Ano: true, Ne: false.");
			Assert::AreEqual(ex.what(), location.data());
		}
	}

	TEST_METHOD(TestWideTextException)
	{
		try
		{
			throw exception::exception(L"Ano: {}, Ne: {}.", std::make_tuple(true, false));
		}
		catch (const std::exception& ex)
		{
			auto location = GetStringLocation(67, 30, "Ano: true, Ne: false.");
			Assert::AreEqual(ex.what(), location.data());
		}

		try
		{
			throw exception::exception(std::source_location::current(), L"Ano: {}, Ne: {}.", true, false);
		}
		catch (const std::exception& ex)
		{
			auto location = GetStringLocation(77, 53, "Ano: true, Ne: false.");
			Assert::AreEqual(ex.what(), location.data());
		}

		try
		{
			throw exception::exception();
		}
		catch (const std::exception& ex)
		{
			auto location = GetStringLocation(87, 30, "");
			Assert::AreEqual(ex.what(), location.data());
		}

		try
		{
			exception::throw_exception(L"Ano: {}, Ne: {}.", true, false);
		}
		catch (const std::exception& ex)
		{
			auto location = GetStringLocation(97, 30, "Ano: true, Ne: false.");
			Assert::AreEqual(ex.what(), location.data());
		}
	}
#else
	TEST_METHOD(TestExceptionNotSupportedConcepts)
	{
	}
#endif
};
} // namespace FrameworkTesting
