#pragma once
#include "extensions/constraints.h"
#include "extensions/finally.h"

#include <atomic>
#include <condition_variable>
#include <semaphore>

namespace janecekvit::synchronization
{

/// <summary>
/// The signal is used for synchronization between different threads.
/// It sets an auto reset state by default, where it resets after each blocking call, or to the manual reset state.
/// It could be used with std::condition_variable_any, std::condition_variable, or std::binary_semaphore.
/// </summary>
#if defined(__cpp_lib_concepts) && defined(__cpp_lib_semaphore)
template <typename _SyncPrimitive = std::binary_semaphore, bool _ManualReset = false>
	requires constraints::semaphore_type<_SyncPrimitive, 1> || constraints::condition_variable_type<_SyncPrimitive>
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
				return _reset_strategy();
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
				return _reset_strategy() || x();
			});
	}

#ifdef __cpp_lib_semaphore
	[[nodiscard]] void wait() const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		for (; !_reset_strategy();)
			_syncPrimitive.acquire();
	}
#endif

#ifdef __cpp_lib_semaphore
	[[nodiscard]] void wait(_Predicate&& pred) const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		for (; !(_reset_strategy() || pred());)
			_syncPrimitive.acquire();
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
					return _reset_strategy() || x();
				});
		}

		return _syncPrimitive.wait_for(lock, rel_time, [&]()
			{
				return _reset_strategy();
			});
	}

#ifdef __cpp_lib_semaphore
	template <class TRep, class TPeriod>
	[[nodiscard]] bool wait_for(const std::chrono::duration<TRep, TPeriod>& rel_time, std::optional<_Predicate>&& pred = {}) const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		if (pred)
		{
			auto x = std::move(*pred);
			for (; !x();)
			{
				if (!_syncPrimitive.try_acquire_for(rel_time))
					return x();
			}
			return true;
		}

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
					return _reset_strategy() || x();
				});
		}

		return _syncPrimitive.wait_until(lock, abs_time, [this]()
			{
				return _reset_strategy();
			});
	}

#ifdef __cpp_lib_semaphore
	template <class TClock, class TDuration>
	[[nodiscard]] bool wait_until(const std::chrono::time_point<TClock, TDuration>& abs_time, std::optional<_Predicate>&& pred = {}) const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		if (pred)
		{
			auto x = std::move(*pred);
			for (; !x();)
			{
				if (!_syncPrimitive.try_acquire_until(abs_time))
					return x();
			}
			return true;
		}

		return _syncPrimitive.try_acquire_until(abs_time);
	}
#endif

	void signalize() noexcept
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
		_signalized = true;
		_syncPrimitive.notify_one();
	}

#ifdef __cpp_lib_semaphore
	void signalize() noexcept
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		_signalized = true;
		_syncPrimitive.release();
	}
#endif

#ifdef __cpp_lib_concepts
	void reset() noexcept
		requires _ManualReset
	{
		_signalized = false;
	}
#endif

private:
	bool _reset_strategy() const noexcept
	{
		if constexpr (_ManualReset)
			return _signalized;

		return _signalized.exchange(false);
	}

private:
	mutable _SyncPrimitive _syncPrimitive;
	mutable std::atomic<bool> _signalized = false;
};

} // namespace janecekvit::synchronization