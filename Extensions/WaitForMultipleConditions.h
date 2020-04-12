#pragma once
#include <atomic>
#include <condition_variable>

#include "Framework/Extensions/Constraints.h"

template <class TCondition = std::condition_variable_any>
class WaitForMultipleConditions
{
public:
	WaitForMultipleConditions() = default;
	virtual ~WaitForMultipleConditions() = default;


	template <class TLock, class TPredicate>
	[[nodiscard]]
	size_t wait(TLock& lock, std::optional<TPredicate>&& pred = {})
	{
		if (pred)
			m_condition.wait(lock, std::move(*pred));
		else
			m_condition.wait(lock);
		return m_uEvent;
	}
	
	template <class TLock, class TRep, class TPeriod, class TPredicate>
	[[nodiscard]]
	std::optional<size_t> wait_for(TLock& lock, const std::chrono::duration<TRep, TPeriod>& rel_time, std::optional<TPredicate>&& pred = {})
	{
		if (pred)
			return (m_condition.wait_for(lock, rel_time, std::move(*pred)) ? m_uEvent : std::nullopt);
		
		return (m_condition.wait_for(lock, rel_time)  == std::cv_status::no_timeout ? m_uEvent : std::nullopt);
	}

	template <class TLock, class TClock, class TDuration, class TPredicate>
	[[nodiscard]]
	std::optional<size_t> wait_until(TLock& lock, const std::chrono::time_point<TClock, TDuration>& timeout_time, std::optional<TPredicate>&& pred = {})
	{
		if (pred)
			return (m_condition.wait_until(lock, timeout_time, std::move(*pred)) ? m_uEvent : std::nullopt);
		
		return (m_condition.wait_until(lock, timeout_time) == std::cv_status::no_timeout ? m_uEvent : std::nullopt);
	}

	void notify_one(size_t uEvent)
	{
		m_uEvent = uEvent;
		m_condition.notify_one();
	}

	void notify_all(size_t uEvent)
	{
		m_uEvent = uEvent;
		m_condition.notify_all();
	}

private:
	TCondition m_condition;
	std::atomic<size_t> m_uEvent = 0;
};

