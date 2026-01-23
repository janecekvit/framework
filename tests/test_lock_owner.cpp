#include "synchronization/lock_owner.h"

#include <chrono>
#include <condition_variable>
#include <gtest/gtest.h>
#include <mutex>
#include <system_error>
#include <utility>

using namespace janecekvit;
using namespace janecekvit::synchronization;

namespace framework_tests
{

class test_lock_owner : public ::testing::Test
{
};

TEST_F(test_lock_owner, TestExclusiveAccess)
{
	synchronization::lock_owner_debug<> owner;
	auto&& lock = owner.exclusive();

	ASSERT_TRUE(lock);
	ASSERT_TRUE(lock.owns_lock());
	ASSERT_TRUE(owner.get_exclusive_lock_details().has_value());

	lock.unlock();
	ASSERT_FALSE(lock);
	ASSERT_FALSE(lock.owns_lock());
	ASSERT_FALSE(owner.get_exclusive_lock_details().has_value());

	lock.lock();
	ASSERT_TRUE(lock);
	ASSERT_TRUE(lock.owns_lock());
	ASSERT_TRUE(owner.get_exclusive_lock_details().has_value());
}

TEST_F(test_lock_owner, TestExclusiveAccessMutex)
{
	synchronization::lock_owner_debug<std::mutex> owner;
	auto&& lock = owner.exclusive();

	ASSERT_TRUE(lock);
	ASSERT_TRUE(lock.owns_lock());
	ASSERT_TRUE(owner.get_exclusive_lock_details().has_value());

	lock.unlock();
	ASSERT_FALSE(lock);
	ASSERT_FALSE(lock.owns_lock());
	ASSERT_FALSE(owner.get_exclusive_lock_details().has_value());
}

TEST_F(test_lock_owner, TestExclusiveAccessScope)
{
	synchronization::lock_owner_debug<> owner;

	{
		auto&& lock = owner.exclusive();
		ASSERT_TRUE(lock);
		ASSERT_TRUE(lock.owns_lock());
		ASSERT_TRUE(owner.get_exclusive_lock_details().has_value());
	}

	ASSERT_FALSE(owner.get_exclusive_lock_details().has_value());
}

TEST_F(test_lock_owner, TestExclusiveAccessTryLock)
{
	synchronization::lock_owner_debug<> owner;

	auto&& lock = owner.exclusive();
	ASSERT_TRUE(lock);

	lock.unlock();
	ASSERT_FALSE(lock);

	ASSERT_TRUE(lock.try_lock());
	ASSERT_TRUE(lock);
}

TEST_F(test_lock_owner, TestExclusiveAccessMove)
{
	synchronization::lock_owner_debug<> owner;

	auto&& lock = owner.exclusive();
	ASSERT_TRUE(lock);
	ASSERT_TRUE(owner.get_exclusive_lock_details().has_value());

	auto lock2 = std::move(lock);
	ASSERT_FALSE(lock);

	ASSERT_TRUE(lock2);
	ASSERT_TRUE(owner.get_exclusive_lock_details().has_value());
}

TEST_F(test_lock_owner, TestExclusiveAccessWait)
{
	synchronization::lock_owner_debug<> owner;
	std::condition_variable_any cv;

	auto&& lock = owner.exclusive();
	ASSERT_TRUE(lock);
	bool bWaited = false;

	lock.wait(cv, [&]
		{
			bWaited = true;
			return true; // Dummy predicate to satisfy the wait
		});

	ASSERT_TRUE(bWaited);
	ASSERT_TRUE(lock);
}

TEST_F(test_lock_owner, TestExclusiveAccessWaitFor)
{
	synchronization::lock_owner_debug<> owner;
	std::condition_variable_any cv;

	auto&& lock = owner.exclusive();
	ASSERT_TRUE(lock);

	auto result = lock.wait_for(cv, std::chrono::microseconds(0));
	ASSERT_EQ(result, std::cv_status::timeout);
	ASSERT_TRUE(lock);
}

TEST_F(test_lock_owner, TestExclusiveAccessDoubleLock)
{
	synchronization::lock_owner_debug<> owner;
	std::condition_variable_any cv;

	auto&& lock = owner.exclusive();
	ASSERT_TRUE(lock);

	ASSERT_THROW(
		{
			lock.lock();
		},
		std::system_error);
}

TEST_F(test_lock_owner, TestExclusiveAccessDoubleTryLock)
{
	synchronization::lock_owner_debug<> owner;
	std::condition_variable_any cv;

	auto&& lock = owner.exclusive();
	ASSERT_TRUE(lock);

	ASSERT_THROW(
		{
			lock.try_lock();
		},
		std::system_error);
}

TEST_F(test_lock_owner, TestExclusiveAccessDoubleUnlock)
{
	synchronization::lock_owner_debug<> owner;
	std::condition_variable_any cv;

	auto&& lock = owner.exclusive();
	ASSERT_TRUE(lock);
	lock.unlock();
	ASSERT_FALSE(lock);

	ASSERT_THROW(
		{
			lock.unlock();
		},
		std::system_error);
}

TEST_F(test_lock_owner, TestConcurrentAccess)
{
	synchronization::lock_owner_debug<> owner;
	auto&& lock = owner.concurrent();

	ASSERT_TRUE(lock);
	ASSERT_TRUE(lock.owns_lock());
	ASSERT_EQ(1, owner.get_concurrent_lock_details().size());

	lock.unlock();
	ASSERT_FALSE(lock);
	ASSERT_FALSE(lock.owns_lock());
	ASSERT_EQ(0, owner.get_concurrent_lock_details().size());

	lock.lock();
	ASSERT_TRUE(lock);
	ASSERT_TRUE(lock.owns_lock());
	ASSERT_EQ(1, owner.get_concurrent_lock_details().size());
}

TEST_F(test_lock_owner, TestConcurrentAccessScope)
{
	synchronization::lock_owner_debug<> owner;
	{
		auto&& lock = owner.concurrent();

		ASSERT_TRUE(lock);
		ASSERT_TRUE(lock.owns_lock());
		ASSERT_EQ(1, owner.get_concurrent_lock_details().size());
	}

	ASSERT_EQ(0, owner.get_concurrent_lock_details().size());
}

TEST_F(test_lock_owner, TestConcurrentAccessTryLock)
{
	synchronization::lock_owner_debug<> owner;

	auto&& lock = owner.concurrent();
	ASSERT_TRUE(lock);

	lock.unlock();
	ASSERT_FALSE(lock);

	ASSERT_TRUE(lock.try_lock());
	ASSERT_TRUE(lock);
}

TEST_F(test_lock_owner, TestConcurrentMultipleAccess)
{
	synchronization::lock_owner_debug<> owner;
	auto&& lock = owner.concurrent();
	auto&& lock2 = owner.concurrent();

	ASSERT_TRUE(lock);
	ASSERT_TRUE(lock.owns_lock());
	ASSERT_TRUE(lock2);
	ASSERT_TRUE(lock2.owns_lock());

	ASSERT_EQ(2, owner.get_concurrent_lock_details().size());

	lock.unlock();
	lock2.unlock();

	ASSERT_FALSE(lock);
	ASSERT_FALSE(lock.owns_lock());
	ASSERT_FALSE(lock2);
	ASSERT_FALSE(lock2.owns_lock());
	ASSERT_EQ(0, owner.get_concurrent_lock_details().size());
}

TEST_F(test_lock_owner, TestConcurentAccessMove)
{
	synchronization::lock_owner_debug<> owner;

	auto&& lock = owner.concurrent();
	ASSERT_TRUE(lock);
	ASSERT_EQ(1, owner.get_concurrent_lock_details().size());

	auto lock2 = std::move(lock);
	ASSERT_FALSE(lock);

	ASSERT_TRUE(lock2);
	ASSERT_EQ(1, owner.get_concurrent_lock_details().size());
}

TEST_F(test_lock_owner, TestConcurrentAccessWait)
{
	synchronization::lock_owner_debug<> owner;
	std::condition_variable_any cv;

	auto&& lock = owner.concurrent();
	ASSERT_TRUE(lock);
	bool bWaited = false;

	lock.wait(cv, [&]
		{
			bWaited = true;
			return true; // Dummy predicate to satisfy the wait
		});

	ASSERT_TRUE(bWaited);
	ASSERT_TRUE(lock);
}

TEST_F(test_lock_owner, TestConcurentAccessWaitFor)
{
	synchronization::lock_owner_debug<> owner;
	std::condition_variable_any cv;

	auto&& lock = owner.concurrent();
	ASSERT_TRUE(lock);

	auto result = lock.wait_for(cv, std::chrono::microseconds(0));
	ASSERT_EQ(result, std::cv_status::timeout);
	ASSERT_TRUE(lock);
}

TEST_F(test_lock_owner, TestConcurrentAccessDoubleLock)
{
	synchronization::lock_owner_debug<> owner;
	std::condition_variable_any cv;

	auto&& lock = owner.concurrent();
	ASSERT_TRUE(lock);

	ASSERT_THROW(
		{
			lock.lock();
		},
		std::system_error);
}

TEST_F(test_lock_owner, TestConcurrentAccessDoubleTryLock)
{
	synchronization::lock_owner_debug<> owner;
	std::condition_variable_any cv;

	auto&& lock = owner.concurrent();
	ASSERT_TRUE(lock);

	ASSERT_THROW(
		{
			lock.try_lock();
		},
		std::system_error);
}

TEST_F(test_lock_owner, TestConcurrentAccessDoubleUnlock)
{
	synchronization::lock_owner_debug<> owner;
	std::condition_variable_any cv;

	auto&& lock = owner.concurrent();
	ASSERT_TRUE(lock);
	lock.unlock();
	ASSERT_FALSE(lock);

	ASSERT_THROW(
		{
			lock.unlock();
		},
		std::system_error);
}

TEST_F(test_lock_owner, TestRuntimePolicyEnableDisable)
{
	synchronization::lock_owner_runtime<> owner;

	synchronization::lock_tracking_runtime::enable_tracking();

	{
		auto&& lock = owner.exclusive();
		(void) lock;
		ASSERT_TRUE(owner.get_exclusive_lock_details().has_value());
	}

	{
		auto&& lock = owner.concurrent();
		(void) lock;
		ASSERT_EQ(1, owner.get_concurrent_lock_details().size());
	}

	synchronization::lock_tracking_runtime::disable_tracking();

	{
		auto&& lock = owner.exclusive();
		(void) lock;
		ASSERT_FALSE(owner.get_exclusive_lock_details().has_value());
	}

	{
		auto&& lock = owner.concurrent();
		(void) lock;
		ASSERT_EQ(0, owner.get_concurrent_lock_details().size());
	}
}

TEST_F(test_lock_owner, TestPolicyMixing)
{
	synchronization::lock_owner_debug<> debug_owner;
	synchronization::lock_owner_release<> release_owner;
	synchronization::lock_owner_runtime<> runtime_owner;

	{
		auto debug_lock = debug_owner.exclusive();
		auto release_lock = release_owner.exclusive();
		auto runtime_lock = runtime_owner.exclusive();

		// Debug always tracks
		ASSERT_TRUE(debug_owner.get_exclusive_lock_details().has_value());

		// runtime tracks depending on configuration (default: disabled)
		ASSERT_FALSE(runtime_owner.get_exclusive_lock_details().has_value());
	}

	{
		auto debug_lock = debug_owner.concurrent();
		auto release_lock = release_owner.concurrent();
		auto runtime_lock = runtime_owner.concurrent();

		// Debug always tracks
		ASSERT_EQ(1, debug_owner.get_concurrent_lock_details().size());

		// Runtime depends on configuration (default: disabled)
		ASSERT_EQ(0, runtime_owner.get_concurrent_lock_details().size());
	}
}

TEST_F(test_lock_owner, TestRuntimePolicyDefault)
{
	synchronization::lock_owner_runtime<> owner;

	{
		auto&& lock = owner.exclusive();
		(void) lock;
		ASSERT_FALSE(owner.get_exclusive_lock_details().has_value());
	}

	{
		auto&& lock = owner.concurrent();
		(void) lock;
		ASSERT_EQ(0, owner.get_concurrent_lock_details().size());
	}
}

TEST_F(test_lock_owner, TestLoggingCallback)
{
	synchronization::lock_tracking_runtime::enable_tracking();

	std::vector<std::tuple<std::string, const void*>> logged_events;

	synchronization::lock_tracking_runtime::set_logging_callback(
		[&logged_events](const lock_information& info, const void* mutex_ptr)
		{
			logged_events.emplace_back(std::string(info.Location.file_name()), mutex_ptr);
		});

	synchronization::lock_owner_runtime<> owner;

	{
		auto&& lock = owner.exclusive();
		(void) lock;
	}

	ASSERT_EQ(1, logged_events.size());
	ASSERT_EQ(owner.get_mutex().get(), std::get<1>(logged_events[0]));

	synchronization::lock_tracking_runtime::clear_logging_callback();
	synchronization::lock_tracking_runtime::disable_tracking();
}

TEST_F(test_lock_owner, TestLoggingCallbackConcurrent)
{
	synchronization::lock_tracking_runtime::enable_tracking();

	int event_count = 0;
	synchronization::lock_tracking_runtime::set_logging_callback(
		[&event_count](const lock_information&, const void*)
		{
			event_count++;
		});

	synchronization::lock_owner_runtime<> owner;

	{
		auto&& lock1 = owner.concurrent();
		auto&& lock2 = owner.concurrent();
		(void) lock1;
		(void) lock2;
	}

	ASSERT_EQ(2, event_count);

	synchronization::lock_tracking_runtime::clear_logging_callback();
	synchronization::lock_tracking_runtime::disable_tracking();
}

TEST_F(test_lock_owner, TestLoggingCallbackTryLock)
{
	synchronization::lock_tracking_runtime::enable_tracking();

	int event_count = 0;

	synchronization::lock_tracking_runtime::set_logging_callback(
		[&event_count](const lock_information&, const void*)
		{
			event_count++;
		});

	synchronization::lock_owner_runtime<> owner;

	{
		auto&& lock = owner.exclusive();
		ASSERT_TRUE(lock.owns_lock());
		lock.unlock();
		ASSERT_TRUE(lock.try_lock());

		lock.unlock();
		auto&& lock2 = owner.exclusive();
		ASSERT_TRUE(lock2.owns_lock());

		ASSERT_FALSE(lock.try_lock());

		(void) lock;
		(void) lock2;
	}
	ASSERT_EQ(3, event_count);

	synchronization::lock_tracking_runtime::clear_logging_callback();
	synchronization::lock_tracking_runtime::disable_tracking();
}

TEST_F(test_lock_owner, TestLoggingCallbackLockInformation)
{
	synchronization::lock_tracking_runtime::enable_tracking();

	std::optional<lock_information> captured_info;

	synchronization::lock_tracking_runtime::set_logging_callback(
		[&captured_info](const lock_information& info, const void*)
		{
			if (!captured_info.has_value())
			{
				captured_info = info;
			}
		});

	synchronization::lock_owner_runtime<> owner;
	int test_line = 0;

	{
		test_line = __LINE__ + 1;
		auto&& lock = owner.exclusive();
		(void) lock;
	}

	ASSERT_TRUE(captured_info.has_value());
	ASSERT_STREQ(captured_info->Location.file_name(), __FILE__);
	ASSERT_EQ(captured_info->Location.line(), static_cast<uint_least32_t>(test_line));
	ASSERT_EQ(std::hash<std::thread::id>()(captured_info->ThreadId), std::hash<std::thread::id>()(std::this_thread::get_id()));
	ASSERT_STREQ(captured_info->MutexType.name(), typeid(*owner.get_mutex()).name());
	ASSERT_FALSE(captured_info->ResourceType.has_value());

	synchronization::lock_tracking_runtime::clear_logging_callback();
	synchronization::lock_tracking_runtime::disable_tracking();
}

TEST_F(test_lock_owner, TestLoggingCallbackMultipleMutexes)
{
	synchronization::lock_tracking_runtime::enable_tracking();

	std::vector<const void*> logged_mutexes;

	synchronization::lock_tracking_runtime::set_logging_callback(
		[&logged_mutexes](const lock_information&, const void* mutex_ptr)
		{
			logged_mutexes.push_back(mutex_ptr);
		});

	synchronization::lock_owner_runtime<> owner1;
	synchronization::lock_owner_runtime<> owner2;

	{
		auto&& lock1 = owner1.exclusive();
		auto&& lock2 = owner2.exclusive();
		(void) lock1;
		(void) lock2;
	}

	ASSERT_EQ(2, logged_mutexes.size());
	ASSERT_EQ(owner1.get_mutex().get(), logged_mutexes[0]);
	ASSERT_EQ(owner2.get_mutex().get(), logged_mutexes[1]);
	ASSERT_NE(logged_mutexes[0], logged_mutexes[1]);

	synchronization::lock_tracking_runtime::clear_logging_callback();
	synchronization::lock_tracking_runtime::disable_tracking();
}

TEST_F(test_lock_owner, TestLoggingCallbackDisabledWhenNoTracking)
{
	using namespace synchronization;

	synchronization::lock_tracking_runtime::disable_tracking();

	int callback_count = 0;

	synchronization::lock_tracking_runtime::set_logging_callback(
		[&callback_count](const lock_information&, const void*)
		{
			callback_count++;
		});

	synchronization::lock_owner_runtime<> owner;

	{
		auto&& lock = owner.exclusive();
		(void) lock;
	}

	ASSERT_EQ(0, callback_count);

	synchronization::lock_tracking_runtime::clear_logging_callback();
}

TEST_F(test_lock_owner, TestLoggingCallbackExceptionSafety)
{
	synchronization::lock_tracking_runtime::enable_tracking();

	synchronization::lock_tracking_runtime::set_logging_callback(
		[](const lock_information&, const void*)
		{
			throw std::runtime_error("Callback exception");
		});

	synchronization::lock_owner_runtime<> owner;

	ASSERT_NO_THROW({
		auto&& lock = owner.exclusive();
		(void) lock;
	});

	{
		auto&& lock = owner.exclusive();
		(void) lock;
		ASSERT_TRUE(owner.get_exclusive_lock_details().has_value());
	}

	synchronization::lock_tracking_runtime::clear_logging_callback();
	synchronization::lock_tracking_runtime::disable_tracking();
}

TEST_F(test_lock_owner, TestLoggingCallbackCompileTimeEnabled)
{
	synchronization::lock_owner_debug<> owner;

	int callback_count = 0;
	std::vector<uint_least32_t> logged_lines;

	synchronization::lock_tracking_enabled::set_logging_callback(
		[&callback_count, &logged_lines](const lock_information& info, const void* mutex_ptr)
		{
			callback_count++;
			logged_lines.push_back(info.Location.line());
			(void) mutex_ptr;
		});

	{
		auto lock = owner.exclusive();
		(void) lock;
	}

	ASSERT_EQ(1, callback_count);
	ASSERT_EQ(1, logged_lines.size());

	synchronization::lock_tracking_enabled::clear_logging_callback();
}

TEST_F(test_lock_owner, TestLoggingCallbackSharedBetweenPolicies)
{
	int callback_count = 0;
	synchronization::lock_owner_debug<> enabled_owner;
	synchronization::lock_owner_runtime<> runtime_owner;

	synchronization::lock_tracking_runtime::enable_tracking();

	synchronization::lock_tracking_enabled::set_logging_callback(
		[&callback_count](const lock_information& info, const void* mutex_ptr)
		{
			callback_count++;
			(void) info;
			(void) mutex_ptr;
		});

	{
		auto lock1 = enabled_owner.exclusive();
		(void) lock1;
	}

	{
		auto lock2 = runtime_owner.exclusive();
		(void) lock2;
	}

	ASSERT_EQ(2, callback_count);

	synchronization::lock_tracking_enabled::clear_logging_callback();
	synchronization::lock_tracking_runtime::disable_tracking();
}

TEST_F(test_lock_owner, TestLoggingCallbackCompileTimeEnabledConcurrent)
{
	int callback_count = 0;
	synchronization::lock_owner_debug<> owner;

	synchronization::lock_tracking_enabled::set_logging_callback(
		[&callback_count](const lock_information& info, const void* mutex_ptr)
		{
			callback_count++;
			(void) info;
			(void) mutex_ptr;
		});

	{
		auto lock1 = owner.concurrent();
		auto lock2 = owner.concurrent();
		(void) lock1;
		(void) lock2;
	}

	ASSERT_EQ(2, callback_count);

	synchronization::lock_tracking_enabled::clear_logging_callback();
}

TEST_F(test_lock_owner, TestLoggingCallbackCompileTimeEnabledDefaultLogging)
{
	lock_owner_debug<> owner;
	lock_tracking_enabled::clear_logging_callback();

	ASSERT_NO_THROW({
		auto lock = owner.exclusive();
		(void) lock;
		ASSERT_TRUE(owner.get_exclusive_lock_details().has_value());
	});
}

TEST_F(test_lock_owner, TestLoggingCallbackCompileTimeEnabledTryLock)
{
	int callback_count = 0;
	synchronization::lock_owner_debug<> owner;

	synchronization::lock_tracking_enabled::set_logging_callback(
		[&callback_count](const lock_information& info, const void* mutex_ptr)
		{
			callback_count++;
			(void) info;
			(void) mutex_ptr;
		});

	{
		auto lock = owner.exclusive();
		lock.unlock();

		ASSERT_TRUE(lock.try_lock());

		lock.unlock();

		auto lock2 = owner.exclusive();
		(void) lock2;

		ASSERT_FALSE(lock.try_lock());
	}

	ASSERT_EQ(3, callback_count);

	synchronization::lock_tracking_enabled::clear_logging_callback();
}

TEST_F(test_lock_owner, TestDefaultLockOwnerAlias)
{
	synchronization::lock_owner<> owner;

	auto lock = owner.exclusive();
	ASSERT_TRUE(lock.owns_lock());
#if defined(SYNCHRONIZATION_RUNTIME_TRACKING)
	ASSERT_FALSE(owner.get_exclusive_lock_details().has_value());
#elif !defined(SYNCHRONIZATION_NO_TRACKING) && !defined(NDEBUG)
	ASSERT_TRUE(owner.get_exclusive_lock_details().has_value());
#endif
}

TEST_F(test_lock_owner, TestDefaultLockOwnerAliasWithMutex)
{
	synchronization::lock_owner<std::mutex> owner;

	auto lock = owner.exclusive();
	ASSERT_TRUE(lock.owns_lock());

#if defined(SYNCHRONIZATION_RUNTIME_TRACKING)
	ASSERT_FALSE(owner.get_exclusive_lock_details().has_value());
#elif !defined(SYNCHRONIZATION_NO_TRACKING) && !defined(NDEBUG)
	ASSERT_TRUE(owner.get_exclusive_lock_details().has_value());
#endif
}

TEST_F(test_lock_owner, TestLockOwnerAliasTypeTraits)
{
	using DefaultOwner = synchronization::lock_owner<>;
	using MutexOwner = synchronization::lock_owner<std::mutex>;

#if defined(NDEBUG) || defined(SYNCHRONIZATION_NO_TRACKING)
	static_assert(std::is_same_v<DefaultOwner, synchronization::lock_owner_release<>>);
	static_assert(std::is_same_v<MutexOwner, synchronization::lock_owner_release<std::mutex>>);
#elif defined(SYNCHRONIZATION_RUNTIME_TRACKING)
	static_assert(std::is_same_v<DefaultOwner, synchronization::lock_owner_runtime<>>);
	static_assert(std::is_same_v<MutexOwner, synchronization::lock_owner_runtime<std::mutex>>);
#else
	static_assert(std::is_same_v<DefaultOwner, synchronization::lock_owner_debug<>>);
	static_assert(std::is_same_v<MutexOwner, synchronization::lock_owner_debug<std::mutex>>);
#endif

	SUCCEED();
}

TEST_F(test_lock_owner, TestDefaultLockOwnerAliasConcurrent)
{
	synchronization::lock_owner<> owner;

	auto lock1 = owner.concurrent();
	auto lock2 = owner.concurrent();

	ASSERT_TRUE(lock1.owns_lock());
	ASSERT_TRUE(lock2.owns_lock());

#if defined(SYNCHRONIZATION_RUNTIME_TRACKING)
	ASSERT_EQ(0, owner.get_concurrent_lock_details().size());
#elif !defined(SYNCHRONIZATION_NO_TRACKING) && !defined(NDEBUG)
	ASSERT_EQ(2, owner.get_concurrent_lock_details().size());
#endif
}

TEST_F(test_lock_owner, TestDefaultLockOwnerAliasDeduction)
{
	synchronization::lock_owner<> owner1;
	auto lock1 = owner1.exclusive();
	ASSERT_TRUE(lock1.owns_lock());

	synchronization::lock_owner<std::mutex> owner2;
	auto lock2 = owner2.exclusive();
	ASSERT_TRUE(lock2.owns_lock());
}

} // namespace framework_tests
