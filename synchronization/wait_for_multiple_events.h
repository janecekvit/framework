#pragma once
#include "extensions/constraints.h"
#include "synchronization/concurrent.h"
#include "synchronization/signal.h"

#include <atomic>
#include <condition_variable>
#include <unordered_set>

namespace janecekvit::synchronization
{

template <class _Condition = std::condition_variable_any>
class wait_for_multiple_events
{
public:
	using TResult = typename std::unordered_set<size_t>;

public:
	wait_for_multiple_events()			= default;
	virtual ~wait_for_multiple_events() = default;

	template <class TLock>
	[[nodiscard]] TResult wait(TLock& lock) const
	{
		_syncPrimitive.wait(lock);
		return _GetResult();
	}

	template <class TLock, class _Predicate>
	[[nodiscard]] TResult wait(TLock& lock, _Predicate&& pred) const
	{
		_syncPrimitive.wait(lock, std::move(pred));
		return _GetResult();
	}

	template <class TLock, class TRep, class TPeriod, class _Predicate>
	[[nodiscard]] TResult wait_for(TLock& lock, const std::chrono::duration<TRep, TPeriod>& rel_time, std::optional<_Predicate>&& pred = {}) const
	{
		if (pred)
			return (_syncPrimitive.wait_for(lock, rel_time, std::move(*pred)) ? _GetResult() : std::nullopt);

		return (_syncPrimitive.wait_for(lock, rel_time) == std::cv_status::no_timeout ? _GetResult() : std::nullopt);
	}

	template <class TLock, class TClock, class TDuration, class _Predicate>
	[[nodiscard]] TResult wait_until(TLock& lock, const std::chrono::time_point<TClock, TDuration>& timeout_time, std::optional<_Predicate>&& pred = {}) const
	{
		if (pred)
			return (_syncPrimitive.wait_until(lock, timeout_time, std::move(*pred)) ? _GetResult() : std::nullopt);

		return (_syncPrimitive.wait_until(lock, timeout_time) == std::cv_status::no_timeout ? _GetResult() : std::nullopt);
	}

	void notify_one(size_t uEvent) noexcept
	{
		m_oEvents.exclusive()->emplace(uEvent);
		_syncPrimitive.signalize();
	}

	void notify_all(size_t uEvent) noexcept
	{
		m_oEvents.exclusive()->emplace(uEvent);
		_syncPrimitive.signalize_all();
	}

private:
	TResult _GetResult() const noexcept
	{
		return m_oEvents.exclusive().move();
	}

private:
	mutable signal<_Condition> _syncPrimitive;
	mutable concurrent::unordered_set<size_t> m_oEvents;
};

} // namespace janecekvit::synchronization