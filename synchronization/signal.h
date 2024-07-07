#pragma once
#include "extensions/constraints.h"
#include "extensions/finally.h"

#include <atomic>
#include <condition_variable>
#include <semaphore>

namespace janecekvit::synchronization
{

/// <summary>
/// Signal is used for synchronization between different threads.
/// It can be set to auto reset state, where it resets after each blocking call, or to manual reset state.
/// It could be used with std::condition_variable_any, std::condition_variable or std::binary_semaphore.
/// </summary>
#if defined(__cpp_lib_concepts) && defined(__cpp_lib_semaphore)
template <typename _SyncPrimitive, ptrdiff_t _Least_max_value = std::_Semaphore_max>
	requires constraints::semaphore_type<_SyncPrimitive, _Least_max_value> || constraints::condition_variable_type<_SyncPrimitive>
#else
template <class _Condition = std::condition_variable_any>
#endif
class signal
{
private:
	using _Predicate = typename std::function<bool()>;

public:
	signal()
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
	}

#ifdef __cpp_lib_semaphore
	signal()
		requires constraints::semaphore_type<_SyncPrimitive>
		: _syncPrimitive(_SyncPrimitive{ 0 })
	{
	}
#endif
	virtual ~signal() = default;

	template <class TLock>
	void wait(TLock& lock) const
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
		_syncPrimitive.wait(lock, [this]()
			{
				return _signalized.exchange(false);
			});
	}

	template <class TLock>
	void wait(TLock& lock, _Predicate&& pred) const
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
		_syncPrimitive.wait(lock, [this, x = std::move(pred)]()
			{
				return _signalized.exchange(false) || x();
			});
	}

#ifdef __cpp_lib_semaphore
	[[nodiscard]] bool wait() const
		requires constraints::semaphore_type<_SyncPrimitive, _Least_max_value>
	{
		return _syncPrimitive.acquire();
	}
#endif

	template <class TLock, class TRep, class TPeriod>
	[[nodiscard]] bool wait_for(TLock& lock, const std::chrono::duration<TRep, TPeriod>& rel_time, std::optional<_Predicate>&& pred = {}) const
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
		if (pred)
		{
			return _syncPrimitive.wait_for(lock, rel_time, [this, x = std::move(*pred)]()
				{
					return _signalized.exchange(false) || x();
				});
		}

		return _syncPrimitive.wait_for(lock, rel_time, [this]()
			{
				return _signalized.exchange(false);
			});
	}

#ifdef __cpp_lib_semaphore
	template <class TRep, class TPeriod>
	[[nodiscard]] bool wait_for(const std::chrono::duration<TRep, TPeriod>& rel_time) const
		requires constraints::semaphore_type<_SyncPrimitive, _Least_max_value>
	{
		return _syncPrimitive.try_acquire_for(rel_time);
	}
#endif

	template <class TLock, class TClock, class TDuration>
	[[nodiscard]] bool wait_until(TLock& lock, const std::chrono::time_point<TClock, TDuration>& abs_time, std::optional<_Predicate>&& pred = {}) const
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
		if (pred)
		{
			return _syncPrimitive.wait_until(lock, abs_time, [this, x = std::move(*pred)]()
				{
					return _signalized.exchange(false) || x();
				});
		}

		return _syncPrimitive.wait_until(lock, abs_time, [this]()
			{
				return _signalized.exchange(false);
			});
	}

#ifdef __cpp_lib_semaphore
	template <class TClock, class TDuration>
	[[nodiscard]] bool wait_until(const std::chrono::time_point<TClock, TDuration>& abs_time) const
		requires constraints::semaphore_type<_SyncPrimitive, _Least_max_value>
	{
		return _syncPrimitive.try_acquire_until(abs_time);
	}
#endif

	void signalize() noexcept
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
		_syncPrimitive.notify_one();
		_signalized = true;
	}

#ifdef __cpp_lib_semaphore
	void signalize() noexcept
		requires constraints::semaphore_type<_SyncPrimitive, _Least_max_value>
	{
		_syncPrimitive.release();
		_signalized = true;
	}
#endif

	void signalize_all() noexcept
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
		_syncPrimitive.notify_all();
		_signalized = true;
	}

private:
	mutable _SyncPrimitive _syncPrimitive;
	mutable std::atomic<bool> _signalized = false;
};

} // namespace janecekvit::synchronization