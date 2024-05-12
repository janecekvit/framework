#pragma once
#include "extensions/constraints.h"
#include "extensions/finally.h"

#include <atomic>
#include <condition_variable>

namespace janecekvit::synchronization
{
/// <summary>
/// Event is used for signalling.
/// It can be set to autoreset state, where it resets after each blocking call, or to manual reset state.
/// </summary>
#ifdef __cpp_lib_concepts
template <constraints::condition_variable_type _Condition = std::condition_variable_any>
#else
template <class _Condition = std::condition_variable_any, bool _AutoReset>
#endif
class event
{
private:
	using _Predicate = typename std::function<bool()>;

public:
	event() = default;
	virtual ~event() = default;

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

//#ifdef __cpp_lib_concepts
//template <ptrdiff_t _MaxValue, constraints::semaphore<_MaxValue> _Semaphore>
//class event
//{
//};
//#endif


} // namespace janecekvit::synchronization