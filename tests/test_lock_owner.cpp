#include "stdafx.h"

#include "synchronization/lock_owner.h"

#include <fstream>
#include <future>
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <thread>

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

} // namespace framework_tests
