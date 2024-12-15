#include "stdafx.h"

#include "CppUnitTest.h"
#include "synchronization/wait_for_multiple_signals.h"

#include <future>
#include <semaphore>
#include <shared_mutex>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace janecekvit;
using namespace std::string_literals;
using namespace std::chrono_literals;

namespace FrameworkTesting
{

ONLY_USED_AT_NAMESPACE_SCOPE class test_wait_for_multiple_signals : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_wait_for_multiple_signals> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
public:
	std::mutex m_conditionMtx;
	enum class TestEnum
	{
		exit,
		ack
	};

	void CompareStatuses(const TestEnum& expected, const TestEnum& actual)
	{
		Assert::AreEqual(static_cast<size_t>(expected), static_cast<size_t>(actual));
	}

	TEST_METHOD(Con_SimpleWait)
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

	TEST_METHOD(Sem_SimpleWait)
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

	TEST_METHOD(Con_WaitFor)
	{
		synchronization::wait_for_multiple_signals<TestEnum, std::condition_variable> wait;
		wait.signalize(TestEnum::ack);
		{
			std::unique_lock lock(m_conditionMtx);
			auto status = wait.wait_for(lock, 100ms);
			Assert::IsTrue(status.has_value());
			CompareStatuses(TestEnum::ack, status.value());
		}

		wait.signalize(TestEnum::exit);
		{
			std::unique_lock lock(m_conditionMtx);
			auto status = wait.wait_for(lock, 100ms, []
				{
					return true;
				});

			Assert::IsTrue(status.has_value());
			CompareStatuses(TestEnum::exit, status.value());
		}
	}

	TEST_METHOD(Sem_WaitFor)
	{
		synchronization::wait_for_multiple_signals<TestEnum> wait;
		wait.signalize(TestEnum::ack);
		{
			auto status = wait.wait_for(100ms);
			Assert::IsTrue(status.has_value());
			CompareStatuses(TestEnum::ack, status.value());
		}

		wait.signalize(TestEnum::exit);
		{
			auto status = wait.wait_for(100ms, []
				{
					return true;
				});

			Assert::IsTrue(status.has_value());
			CompareStatuses(TestEnum::exit, status.value());
		}
	}

	TEST_METHOD(Con_WaitUntil)
	{
		synchronization::wait_for_multiple_signals<TestEnum, std::condition_variable> wait;
		wait.signalize(TestEnum::ack);
		{
			std::unique_lock lock(m_conditionMtx);
			auto status = wait.wait_until(lock, std::chrono::steady_clock::now());
			Assert::IsTrue(status.has_value());
			CompareStatuses(TestEnum::ack, status.value());
		}

		wait.signalize(TestEnum::exit);
		{
			std::unique_lock lock(m_conditionMtx);
			auto status = wait.wait_until(lock, std::chrono::steady_clock::now(), []
				{
					return true;
				});

			Assert::IsTrue(status.has_value());
			CompareStatuses(TestEnum::exit, status.value());
		}
	}

	TEST_METHOD(Sem_WaitUntil)
	{
		synchronization::wait_for_multiple_signals<TestEnum> wait;
		wait.signalize(TestEnum::ack);
		{
			auto status = wait.wait_until(std::chrono::steady_clock::now());
			Assert::IsTrue(status.has_value());
			CompareStatuses(TestEnum::ack, status.value());
		}

		wait.signalize(TestEnum::exit);
		{
			auto status = wait.wait_until(std::chrono::steady_clock::now(), []
				{
					return true;
				});

			Assert::IsTrue(status.has_value());
			CompareStatuses(TestEnum::exit, status.value());
		}
	}
};

} // namespace FrameworkTesting