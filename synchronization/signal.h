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
	using _predicate = typename std::function<bool()>;

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
		: _primitive(_SyncPrimitive{ 0 })
	{
	}
#endif
	virtual ~signal() = default;

	template <class _TLock>
	void wait(_TLock& lock) const
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
		_primitive.wait(lock, [this]()
			{
				return _reset_strategy();
			});
	}

	template <class _TLock>
	void wait(_TLock& lock, _predicate&& pred) const
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
		_primitive.wait(lock, [this, x = std::move(pred)]()
			{
				return _reset_strategy() || x();
			});
	}

#ifdef __cpp_lib_semaphore
	void wait() const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		for (; !_reset_strategy();)
			_primitive.acquire();
	}
#endif

#ifdef __cpp_lib_semaphore
	void wait(_predicate&& pred) const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		for (; !(_reset_strategy() || pred());)
			_primitive.acquire();
	}
#endif

	template <class _TLock, class TRep, class TPeriod>
	[[nodiscard]] bool wait_for(_TLock& lock, const std::chrono::duration<TRep, TPeriod>& rel_time, std::optional<_predicate>&& pred = {}) const
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
		if (pred)
		{
			return _primitive.wait_for(lock, rel_time, [this, x = std::move(*pred)]()
				{
					return _reset_strategy() || x();
				});
		}

		return _primitive.wait_for(lock, rel_time, [&]()
			{
				return _reset_strategy();
			});
	}

#ifdef __cpp_lib_semaphore
	template <class TRep, class TPeriod>
	[[nodiscard]] bool wait_for(const std::chrono::duration<TRep, TPeriod>& rel_time, std::optional<_predicate>&& pred = {}) const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		if (pred)
		{
			auto x = std::move(*pred);
			for (; !x();)
			{
				if (!_primitive.try_acquire_for(rel_time))
					return x();
			}
			return true;
		}

		return _primitive.try_acquire_for(rel_time);
	}
#endif

	template <class _TLock, class _TClock, class _TDuration>
	[[nodiscard]] bool wait_until(_TLock& lock, const std::chrono::time_point<_TClock, _TDuration>& abs_time, std::optional<_predicate>&& pred = {}) const
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
		if (pred)
		{
			return _primitive.wait_until(lock, abs_time, [this, x = std::move(*pred)]()
				{
					return _reset_strategy() || x();
				});
		}

		return _primitive.wait_until(lock, abs_time, [this]()
			{
				return _reset_strategy();
			});
	}

#ifdef __cpp_lib_semaphore
	template <class _TClock, class _TDuration>
	[[nodiscard]] bool wait_until(const std::chrono::time_point<_TClock, _TDuration>& abs_time, std::optional<_predicate>&& pred = {}) const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		if (pred)
		{
			auto x = std::move(*pred);
			for (; !x();)
			{
				if (!_primitive.try_acquire_until(abs_time))
					return x();
			}
			return true;
		}

		return _primitive.try_acquire_until(abs_time);
	}
#endif

	void signalize() noexcept
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
		_signalized = true;
		if constexpr (_ManualReset)
			_primitive.notify_all();
		else
			_primitive.notify_one();
	}

	void signalize_all() noexcept
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
		_signalized = true;
		_primitive.notify_all();
	}

#ifdef __cpp_lib_semaphore
	void signalize() noexcept
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		_signalized = true;
		_primitive.release();
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
		else
			return _signalized.exchange(false);
	}

private:
	mutable _SyncPrimitive _primitive;
	mutable std::atomic<bool> _signalized = false;
};

#ifdef __cpp_lib_concepts
template <class _Type, class _InnerType = std::binary_semaphore>
concept signal_type = std::is_same_v<_Type, janecekvit::synchronization::signal<_InnerType>>;
#endif

} // namespace janecekvit::synchronization
