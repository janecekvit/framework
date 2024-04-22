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
#if (__cplusplus >= __cpp_lib_concepts)
	TEST_METHOD(TestSimpleTextException)
	{
		try
		{
			throw exception::exception("Ano: {}, Ne: {}.", std::make_tuple(true, false));
		}
		catch (const std::exception& ex)
		{
			Assert::AreEqual(ex.what(), R"(File: C:\Users\DeWitt\Source\Repos\Framework\Testing\test_exception.cpp(18:30) 'TestSimpleTextException'. Ano: true, Ne: false.)");
		}

		try
		{
			throw exception::exception(std::source_location::current(), "Ano: {}, Ne: {}.", true, false);
		}
		catch (const std::exception& ex)
		{
			Assert::AreEqual(ex.what(), R"(File: C:\Users\DeWitt\Source\Repos\Framework\Testing\test_exception.cpp(27:53) 'TestSimpleTextException'. Ano: true, Ne: false.)");
		}

		try
		{
			throw exception::exception();
		}
		catch (const std::exception& ex)
		{
			Assert::AreEqual(ex.what(), R"(File: C:\Users\DeWitt\Source\Repos\Framework\Testing\test_exception.cpp(36:30) 'TestSimpleTextException'. )");
		}

		try
		{
			exception::throw_exception("Ano: {}, Ne: {}.", true, false);
		}
		catch (const std::exception& ex)
		{
			Assert::AreEqual(ex.what(), R"(File: C:\Users\DeWitt\Source\Repos\Framework\Testing\test_exception.cpp(45:30) 'TestSimpleTextException'. Ano: true, Ne: false.)");
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
			Assert::AreEqual(ex.what(), R"(File: C:\Users\DeWitt\Source\Repos\Framework\Testing\test_exception.cpp(57:30) 'TestWideTextException'. Ano: true, Ne: false.)");
		}

		try
		{
			throw exception::exception(std::source_location::current(), L"Ano: {}, Ne: {}.", true, false);
		}
		catch (const std::exception& ex)
		{
			Assert::AreEqual(ex.what(), R"(File: C:\Users\DeWitt\Source\Repos\Framework\Testing\test_exception.cpp(66:53) 'TestWideTextException'. Ano: true, Ne: false.)");
		}

		try
		{
			throw exception::exception();
		}
		catch (const std::exception& ex)
		{
			Assert::AreEqual(ex.what(), R"(File: C:\Users\DeWitt\Source\Repos\Framework\Testing\test_exception.cpp(75:30) 'TestWideTextException'. )");
		}

		try
		{
			exception::throw_exception(L"Ano: {}, Ne: {}.", true, false);
		}
		catch (const std::exception& ex)
		{
			Assert::AreEqual(ex.what(), R"(File: C:\Users\DeWitt\Source\Repos\Framework\Testing\test_exception.cpp(84:30) 'TestWideTextException'. Ano: true, Ne: false.)");
		}
	}
#else
	TEST_METHOD(TestExceptionNotSupportedConcepts)
	{
	}
#endif
};
} // namespace FrameworkTesting
