#pragma once
#include "extensions/constraints.h"
#include "extensions/finally.h"

#include <atomic>
#include <condition_variable>

namespace janecekvit::synchronization
{

template <class _Condition = std::condition_variable_any>
class AtomicConditionVariable
{
private:
	using _Predicate = typename std::function<bool()>;

public:
	AtomicConditionVariable()		   = default;
	virtual ~AtomicConditionVariable() = default;

	template <class TLock>
	void wait(TLock& lock) const
	{
		m_condition.wait(lock, [this]()
			{
				return m_bSignalized.exchange(false);
			});
	}

	template <class TLock>
	void wait(TLock& lock, _Predicate&& pred) const
	{
		m_condition.wait(lock, [this, x = std::move(pred)]()
			{
				return m_bSignalized.exchange(false) || x();
			});
	}

	template <class TLock, class TRep, class TPeriod>
	[[nodiscard]] bool wait_for(TLock& lock, const std::chrono::duration<TRep, TPeriod>& rel_time, std::optional<_Predicate>&& pred = {}) const
	{
		if (pred)
		{
			return m_condition.wait_for(lock, rel_time, [this, x = std::move(*pred)]()
				{
					return m_bSignalized.exchange(false) || x();
				});
		}

		return m_condition.wait_for(lock, rel_time, [this]()
			{
				return m_bSignalized.exchange(false);
			});
	}

	template <class TLock, class TClock, class TDuration>
	[[nodiscard]] bool wait_until(TLock& lock, const std::chrono::time_point<TClock, TDuration>& timeout_time, std::optional<_Predicate>&& pred = {}) const
	{
		if (pred)
		{
			return m_condition.wait_until(lock, timeout_time, [this, x = std::move(*pred)]()
				{
					return m_bSignalized.exchange(false) || x();
				});
		}

		return m_condition.wait_until(lock, timeout_time, [this]()
			{
				return m_bSignalized.exchange(false);
			});
	}

	void notify_one() noexcept
	{
		m_condition.notify_one();
		m_bSignalized = true;
	}

	void notify_all() noexcept
	{
		m_condition.notify_all();
		m_bSignalized = true;
	}

private:
	mutable _Condition m_condition;
	mutable std::atomic<bool> m_bSignalized = false;
};

} // namespace janecekvit::synchronization