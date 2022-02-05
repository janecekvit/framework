#include "stdafx.h"

#include "CppUnitTest.h"
#include "tracing/trace.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace janecekvit;

namespace FrameworkTesting
{
ONLY_USED_AT_NAMESPACE_SCOPE class test_trace : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_trace> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
public:
	enum class test
	{
		no,
		yes
	};
	TEST_METHOD(TestTrace)
	{
		tracing::trace<std::wstring, test> trace;

		tracing::trace_event e{ test::yes, std::this_thread::get_id(), L"yes" };

		//trace.create(tracing::trace_event{ test::yes, std::this_thread::get_id(), L"yes" });
	}
};
} // namespace FrameworkTesting
