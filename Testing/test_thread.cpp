#include "stdafx.h"

#include "CppUnitTest.h"
#include "Thread/sync_thread_pool.h"

#include <future>
#include <iostream>
#include <string>
using namespace janecekvit::thread;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FrameworkTesting
{

ONLY_USED_AT_NAMESPACE_SCOPE class TestThread : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<TestThread>
{
public:
	const size_t thread_size = 4;
	TEST_METHOD(SyncThreadInit)
	{
		sync_thread_pool pool(thread_size);
		Assert::AreEqual(pool.pool_size(), thread_size);
	} // namespace FrameworkTesting
};
} // namespace FrameworkTesting
