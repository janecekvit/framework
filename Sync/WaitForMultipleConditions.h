#pragma once
#include "Extensions/Concurrent.h"
#include "Extensions/constraints.h"
#include "Sync/AtomicConditionVariable.h"

#include <atomic>
#include <condition_variable>
#include <unordered_set>

template <class _Condition = std::condition_variable_any>
class WaitForMultipleConditions
{
public:
	using TResult = typename std::unordered_set<size_t>;

public:
	WaitForMultipleConditions()			 = default;
	virtual ~WaitForMultipleConditions() = default;

	template <class TLock>
	[[nodiscard]] TResult wait(TLock& lock) const
	{
		m_condition.wait(lock);
		return _GetResult();
	}

	template <class TLock, class _Predicate>
	[[nodiscard]] TResult wait(TLock& lock, _Predicate&& pred) const
	{
		m_condition.wait(lock, std::move(pred));
		return _GetResult();
	}

	template <class TLock, class TRep, class TPeriod, class _Predicate>
	[[nodiscard]] TResult wait_for(TLock& lock, const std::chrono::duration<TRep, TPeriod>& rel_time, std::optional<_Predicate>&& pred = {}) const
	{
		if (pred)
			return (m_condition.wait_for(lock, rel_time, std::move(*pred)) ? _GetResult() : std::nullopt);

		return (m_condition.wait_for(lock, rel_time) == std::cv_status::no_timeout ? _GetResult() : std::nullopt);
	}

	template <class TLock, class TClock, class TDuration, class _Predicate>
	[[nodiscard]] TResult wait_until(TLock& lock, const std::chrono::time_point<TClock, TDuration>& timeout_time, std::optional<_Predicate>&& pred = {}) const
	{
		if (pred)
			return (m_condition.wait_until(lock, timeout_time, std::move(*pred)) ? _GetResult() : std::nullopt);

		return (m_condition.wait_until(lock, timeout_time) == std::cv_status::no_timeout ? _GetResult() : std::nullopt);
	}

	void notify_one(size_t uEvent) noexcept
	{
		m_oEvents.exclusive()->emplace(uEvent);
		m_condition.notify_one();
	}

	void notify_all(size_t uEvent) noexcept
	{
		m_oEvents.exclusive()->emplace(uEvent);
		m_condition.notify_all();
	}

private:
	TResult _GetResult() const noexcept
	{
		return m_oEvents.exclusive().move();
	}

private:
	mutable AtomicConditionVariable<_Condition> m_condition;
	mutable concurrent::unordered_set<size_t> m_oEvents;
};
