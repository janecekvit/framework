#include "tracing/trace.h"

#include <gtest/gtest.h>

using namespace janecekvit;
using namespace std::string_literals;

namespace framework_tests
{
class test_trace : public ::testing::Test
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

enum class test
{
	Warning,
	Verbose
};

TEST_F(test_trace, TestTrace)
{
	tracing::trace<std::wstring, test> trace;

	tracing::trace_event e{ test::Warning, L"ANO: {}", true };

	trace.create(std::move(e));

	trace.create(tracing::trace_event{ test::Verbose, L"NE: {}", false });

	ASSERT_EQ(trace.size(), static_cast<size_t>(2));
	auto&& t1 = trace.next_trace();
	auto&& t2 = trace.next_trace();
	ASSERT_EQ(trace.size(), static_cast<size_t>(0));

	// implicit conversion
	test t = t1;
	std::thread::id id = t1;
	std::source_location srcl = t1;
	std::wstring data = t1;

	// get default source location to retrieve filename
	auto defaultLocation = std::source_location::current();

	ASSERT_EQ(static_cast<size_t>(t), static_cast<size_t>(test::Warning));
	ASSERT_EQ(std::hash<std::thread::id>()(id), std::hash<std::thread::id>()(std::this_thread::get_id()));
	ASSERT_EQ(srcl.file_name(), defaultLocation.file_name());
	ASSERT_EQ(srcl.function_name(), srcl.function_name());
	ASSERT_EQ(srcl.line(), static_cast<uint_least32_t>(35));
	ASSERT_EQ(data, L"ANO: true"s);

	ASSERT_EQ(static_cast<size_t>(t2.priority()), static_cast<size_t>(test::Verbose));
	ASSERT_EQ(std::hash<std::thread::id>()(t2.thread_id()), std::hash<std::thread::id>()(std::this_thread::get_id()));
	ASSERT_EQ(t2.source_location().file_name(), defaultLocation.file_name());
	ASSERT_EQ(t2.source_location().function_name(), defaultLocation.function_name());
	ASSERT_EQ(t2.source_location().line(), static_cast<uint_least32_t>(39));
	ASSERT_EQ(t2.data(), L"NE: false"s);
}
#else
TEST_F(TestTraceNotSupportedConcepts)
{
}
#endif

} // namespace framework_tests
