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
	TEST_METHOD(TestSimpleTextException)
	{
		try
		{
			throw exception::exception("Ano: {}, Ne: {}.", { true, false });
		}
		catch (const exception::exception&)
		{
		}
		//Assert::AreEqual<int>(size, 4);
	}

	TEST_METHOD(TestWideTextException)
	{
		//Assert::AreEqual<int>(size, 4);
	}
};
} // namespace FrameworkTesting
