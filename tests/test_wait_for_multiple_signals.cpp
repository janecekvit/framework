#include "synchronization/wait_for_multiple_signals.h"

#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <semaphore>
#include <shared_mutex>

using namespace janecekvit;
using namespace std::string_literals;
using namespace std::chrono_literals;

namespace framework_tests
{

class test_wait_for_multiple_signals : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}
};

std::mutex m_conditionMtx;
enum class TestEnum
{
	exit,
	ack
};

void CompareStatuses(const TestEnum& expected, const TestEnum& actual)
{
	ASSERT_EQ(static_cast<size_t>(expected), static_cast<size_t>(actual));
}

TEST_F(test_wait_for_multiple_signals, Con_SimpleWait)
{
	synchronization::wait_for_multiple_signals<TestEnum, std::condition_variable> wait;
	wait.signalize(TestEnum::ack);
	{
		std::unique_lock lock(m_conditionMtx);
		auto status = wait.wait(lock);
		CompareStatuses(TestEnum::ack, status);
	}

	wait.signalize(TestEnum::exit);
	{
		std::unique_lock lock(m_conditionMtx);
		auto status = wait.wait(lock, []
			{
				return true;
			});

		CompareStatuses(TestEnum::exit, status);
	}
}

TEST_F(test_wait_for_multiple_signals, Sem_SimpleWait)
{
	synchronization::wait_for_multiple_signals<TestEnum> wait;
	wait.signalize(TestEnum::ack);
	auto status = wait.wait();
	CompareStatuses(TestEnum::ack, status);

	wait.signalize(TestEnum::exit);
	status = wait.wait([]
		{
			return true;
		});

	CompareStatuses(TestEnum::exit, status);
}

TEST_F(test_wait_for_multiple_signals, Con_WaitFor)
{
	synchronization::wait_for_multiple_signals<TestEnum, std::condition_variable> wait;
	wait.signalize(TestEnum::ack);
	{
		std::unique_lock lock(m_conditionMtx);
		auto status = wait.wait_for(lock, 100ms);
		ASSERT_TRUE(status.has_value());
		CompareStatuses(TestEnum::ack, status.value());
	}

	wait.signalize(TestEnum::exit);
	{
		std::unique_lock lock(m_conditionMtx);
		auto status = wait.wait_for(lock, 100ms, []
			{
				return true;
			});

		ASSERT_TRUE(status.has_value());
		CompareStatuses(TestEnum::exit, status.value());
	}
}

TEST_F(test_wait_for_multiple_signals, Sem_WaitFor)
{
	synchronization::wait_for_multiple_signals<TestEnum> wait;
	wait.signalize(TestEnum::ack);
	{
		auto status = wait.wait_for(100ms);
		ASSERT_TRUE(status.has_value());
		CompareStatuses(TestEnum::ack, status.value());
	}

	wait.signalize(TestEnum::exit);
	{
		auto status = wait.wait_for(100ms, []
			{
				return true;
			});

		ASSERT_TRUE(status.has_value());
		CompareStatuses(TestEnum::exit, status.value());
	}
}

TEST_F(test_wait_for_multiple_signals, Con_WaitUntil)
{
	synchronization::wait_for_multiple_signals<TestEnum, std::condition_variable> wait;
	wait.signalize(TestEnum::ack);
	{
		std::unique_lock lock(m_conditionMtx);
		auto status = wait.wait_until(lock, std::chrono::steady_clock::now());
		ASSERT_TRUE(status.has_value());
		CompareStatuses(TestEnum::ack, status.value());
	}

	wait.signalize(TestEnum::exit);
	{
		std::unique_lock lock(m_conditionMtx);
		auto status = wait.wait_until(lock, std::chrono::steady_clock::now(), []
			{
				return true;
			});

		ASSERT_TRUE(status.has_value());
		CompareStatuses(TestEnum::exit, status.value());
	}
}

TEST_F(test_wait_for_multiple_signals, Sem_WaitUntil)
{
	synchronization::wait_for_multiple_signals<TestEnum> wait;
	wait.signalize(TestEnum::ack);
	{
		auto status = wait.wait_until(std::chrono::steady_clock::now());
		ASSERT_TRUE(status.has_value());
		CompareStatuses(TestEnum::ack, status.value());
	}

	wait.signalize(TestEnum::exit);
	{
		auto status = wait.wait_until(std::chrono::steady_clock::now(), []
			{
				return true;
			});

		ASSERT_TRUE(status.has_value());
		CompareStatuses(TestEnum::exit, status.value());
	}
}

} // namespace framework_tests
