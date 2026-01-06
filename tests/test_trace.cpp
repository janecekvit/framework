#include <gtest/gtest.h>
#include "compatibility/compiler_support.h"

#if defined(HAS_STD_FORMAT)

#include "tracing/trace.h"

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
	auto t1_opt = trace.get_next_trace();
	auto t2_opt = trace.get_next_trace();
	ASSERT_EQ(trace.size(), static_cast<size_t>(0));

	ASSERT_TRUE(t1_opt.has_value());
	ASSERT_TRUE(t2_opt.has_value());

	auto& t1 = *t1_opt;
	auto& t2 = *t2_opt;

	// implicit conversion
	test t = t1;
	std::thread::id id = t1;
	std::source_location srcl = t1;
	std::wstring data = t1;

	// get default source location to retrieve filename
	auto defaultLocation = std::source_location::current();

	ASSERT_EQ(static_cast<size_t>(t), static_cast<size_t>(test::Warning));
	ASSERT_EQ(std::hash<std::thread::id>()(id), std::hash<std::thread::id>()(std::this_thread::get_id()));
#ifndef __APPLE__
	ASSERT_STREQ(srcl.file_name(), defaultLocation.file_name());
	ASSERT_STREQ(srcl.function_name(), defaultLocation.function_name());
	ASSERT_EQ(srcl.line(), static_cast<uint_least32_t>(35));
#endif
	ASSERT_EQ(data, L"ANO: true"s);

	ASSERT_EQ(static_cast<size_t>(t2.priority()), static_cast<size_t>(test::Verbose));
	ASSERT_EQ(std::hash<std::thread::id>()(t2.thread_id()), std::hash<std::thread::id>()(std::this_thread::get_id()));
#ifndef __APPLE__
	ASSERT_STREQ(t2.source_location().file_name(), defaultLocation.file_name());

	ASSERT_STREQ(t2.source_location().function_name(), defaultLocation.function_name());
	ASSERT_EQ(t2.source_location().line(), static_cast<uint_least32_t>(39));
#endif
	ASSERT_EQ(t2.data(), L"NE: false"s);

	auto t3_opt = trace.get_next_trace();
	ASSERT_FALSE(t3_opt.has_value());
}

TEST_F(test_trace, TestTraceWait)
{
	tracing::trace<std::wstring, test> trace;

	std::thread producer([&trace]()
		{
			trace.create(tracing::trace_event{ test::Warning, L"Delayed event: {}", 42 });
		});

	auto evt = trace.get_next_trace_wait();

	producer.join();

	ASSERT_EQ(evt.data(), L"Delayed event: 42"s);
	ASSERT_EQ(static_cast<size_t>(evt.priority()), static_cast<size_t>(test::Warning));
}

TEST_F(test_trace, TestTraceWaitFor)
{
	tracing::trace<std::wstring, test> trace;

	auto evt_timeout = trace.get_next_trace_wait_for(std::chrono::milliseconds(0));

	ASSERT_FALSE(evt_timeout.has_value());

	trace.create(tracing::trace_event{ test::Verbose, L"Timed event: {}", true });

	auto evt = trace.get_next_trace_wait_for(std::chrono::milliseconds(200));

	ASSERT_TRUE(evt.has_value());
	ASSERT_EQ(evt->data(), L"Timed event: true"s);
	ASSERT_EQ(static_cast<size_t>(evt->priority()), static_cast<size_t>(test::Verbose));
}

} // namespace framework_tests

#endif
