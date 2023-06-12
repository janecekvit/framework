#include "stdafx.h"

#include "CppUnitTest.h"
#include "tracing/trace.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace janecekvit;
using namespace std::string_literals;

namespace FrameworkTesting
{
ONLY_USED_AT_NAMESPACE_SCOPE class test_trace : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_trace> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
public:
#if (__cplusplus > __cpp_lib_concepts)

	enum class test{
		Warning,
		Verbose
	};
	TEST_METHOD(TestTrace)
	{
		tracing::trace<std::wstring, test> trace;

		tracing::trace_event e{ test::Warning, L"ANO: {}", true };

		trace.create(std::move(e));

		trace.create(tracing::trace_event{ test::Verbose, L"NE: {}", false });

		Assert::AreEqual(trace.size(), static_cast<size_t>(2));
		auto&& t1 = trace.next_trace();
		auto&& t2 = trace.next_trace();
		Assert::AreEqual(trace.size(), static_cast<size_t>(0));

		test t					  = t1;
		std::thread::id id		  = t1;
		std::source_location srcl = t1;
		std::wstring data		  = t1;

		Assert::AreEqual(static_cast<size_t>(t), static_cast<size_t>(test::Warning));
		Assert::AreEqual(std::hash<std::thread::id>()(id), std::hash<std::thread::id>()(std::this_thread::get_id()));
		Assert::AreEqual(srcl.file_name(), R"(C:\Users\DeWitt\Source\Repos\Framework\Testing\test_trace.cpp)");
		Assert::AreEqual(srcl.function_name(), "TestTrace");
		Assert::AreEqual(srcl.line(), static_cast<uint_least32_t>(24));
		Assert::AreEqual(data, L"ANO: true"s);

		Assert::AreEqual(static_cast<size_t>(t2.priority()), static_cast<size_t>(test::Verbose));
		Assert::AreEqual(std::hash<std::thread::id>()(t2.thread_id()), std::hash<std::thread::id>()(std::this_thread::get_id()));
		Assert::AreEqual(t2.source_location().file_name(), R"(C:\Users\DeWitt\Source\Repos\Framework\Testing\test_trace.cpp)");
		Assert::AreEqual(t2.source_location().function_name(), "TestTrace");
		Assert::AreEqual(t2.source_location().line(), static_cast<uint_least32_t>(28));
		Assert::AreEqual(t2.data(), L"NE: false"s);
	}
#else
	TEST_METHOD(TestTraceNotSupportedConcepts)
	{
	}
#endif
};
} // namespace FrameworkTesting
