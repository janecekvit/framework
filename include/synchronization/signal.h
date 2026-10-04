#pragma once
#include "extensions/constraints.h"
#include "extensions/finally.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <semaphore>
#include <tuple>

namespace janecekvit::synchronization
{

/// <summary>
/// The signal is used for synchronization between different threads.
/// It sets an auto reset state by default, where it resets after each blocking call, or to the manual reset state.
/// It could be used with std::condition_variable_any, std::condition_variable, or std::binary_semaphore.
/// Manual reset works only with std::condition_variable_any, std::condition_variable.
/// </summary>
template <typename _SyncPrimitive = std::binary_semaphore, bool _ManualReset = false>
	requires(constraints::semaphore_type<_SyncPrimitive, 1> || constraints::condition_variable_type<_SyncPrimitive>) && (!(constraints::semaphore_type<_SyncPrimitive> && _ManualReset))
class signal
{
public:
	signal()
		requires constraints::condition_variable_type<_SyncPrimitive>
	{
	}

	signal()
		requires constraints::semaphore_type<_SyncPrimitive>
		: _primitive(_SyncPrimitive{ 0 })
	{
	}

	virtual ~signal() = default;

	template <class _TLock>
	void wait(_TLock& lock) const
		requires constraints::condition_variable_type<_SyncPrimitive>
	{
		wait(lock, _no_predicate);
	}

	template <class _TLock, class _PredicateType>
	void wait(_TLock& lock, _PredicateType&& pred) const
		requires constraints::condition_variable_type<_SyncPrimitive> && std::predicate<_PredicateType>
	{
		const auto initial_version = _get_initial_version();

		_wait(lock, [this, initial_version, predicate = std::forward<_PredicateType>(pred)]() -> bool
			{
				return predicate() || _check_signal(initial_version);
			});
	}

	void wait() const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		wait(_no_predicate);
	}

	template <class _PredicateType>
	void wait(_PredicateType&& pred) const
		requires constraints::semaphore_type<_SyncPrimitive, 1> && std::predicate<_PredicateType>
	{
		const auto initial_version = _get_initial_version();

		while (!(pred() || _check_signal(initial_version)))
			_primitive.acquire();
	}

	template <class _TLock, class TRep, class TPeriod>
	[[nodiscard]] bool wait_for(_TLock& lock, const std::chrono::duration<TRep, TPeriod>& rel_time) const
		requires constraints::condition_variable_type<_SyncPrimitive>
	{
		return wait_until(lock, std::chrono::steady_clock::now() + rel_time, _no_predicate);
	}

	template <class _TLock, class TRep, class TPeriod, class _PredicateType>
	[[nodiscard]] bool wait_for(_TLock& lock, const std::chrono::duration<TRep, TPeriod>& rel_time, _PredicateType&& pred) const
		requires constraints::condition_variable_type<_SyncPrimitive> && std::predicate<_PredicateType>
	{
		return wait_until(lock, std::chrono::steady_clock::now() + rel_time, std::forward<_PredicateType>(pred));
	}

	template <class TRep, class TPeriod>
	[[nodiscard]] bool wait_for(const std::chrono::duration<TRep, TPeriod>& rel_time) const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		return wait_until(std::chrono::steady_clock::now() + rel_time, _no_predicate);
	}

	template <class TRep, class TPeriod, class _PredicateType>
	[[nodiscard]] bool wait_for(const std::chrono::duration<TRep, TPeriod>& rel_time, _PredicateType&& pred) const
		requires constraints::semaphore_type<_SyncPrimitive, 1> && std::predicate<_PredicateType>
	{
		return wait_until(std::chrono::steady_clock::now() + rel_time, std::forward<_PredicateType>(pred));
	}

	template <class _TLock, class _TClock, class _TDuration>
	[[nodiscard]] bool wait_until(_TLock& lock, const std::chrono::time_point<_TClock, _TDuration>& abs_time) const
		requires constraints::condition_variable_type<_SyncPrimitive>
	{
		return wait_until(lock, abs_time, _no_predicate);
	}

	template <class _TLock, class _TClock, class _TDuration, class _PredicateType>
	[[nodiscard]] bool wait_until(_TLock& lock, const std::chrono::time_point<_TClock, _TDuration>& abs_time, _PredicateType&& pred) const
		requires constraints::condition_variable_type<_SyncPrimitive> && std::predicate<_PredicateType>
	{
		const auto initial_version = _get_initial_version();

		return _wait_until(lock, abs_time, [this, initial_version, predicate = std::forward<_PredicateType>(pred)]() -> bool
			{
				return predicate() || _check_signal(initial_version);
			});
	}

	template <class _TClock, class _TDuration>
	[[nodiscard]] bool wait_until(const std::chrono::time_point<_TClock, _TDuration>& abs_time) const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		return wait_until(abs_time, _no_predicate);
	}

	template <class _TClock, class _TDuration, class _PredicateType>
	[[nodiscard]] bool wait_until(const std::chrono::time_point<_TClock, _TDuration>& abs_time, _PredicateType&& pred) const
		requires constraints::semaphore_type<_SyncPrimitive, 1> && std::predicate<_PredicateType>
	{
		auto predicate = std::forward<_PredicateType>(pred);
		const auto initial_version = _get_initial_version();

		while (!(predicate() || _check_signal(initial_version)))
		{
			if (!_primitive.try_acquire_until(abs_time))
				return predicate() || _check_signal(initial_version);
		}
		return true;
	}

	void signalize() noexcept
		requires constraints::condition_variable_type<_SyncPrimitive>
	{
		{
			std::lock_guard lock(_state.state_mutex);
			_state.signalized.store(true, std::memory_order_release);
			_state.signal_version.fetch_add(1, std::memory_order_acq_rel);
		}

		if constexpr (_ManualReset)
			_primitive.notify_all();
		else
			_primitive.notify_one();
	}

	void signalize_all() noexcept
		requires constraints::condition_variable_type<_SyncPrimitive>
	{
		{
			std::lock_guard lock(_state.state_mutex);
			_state.signal_version.fetch_add(1, std::memory_order_acq_rel);

			// auto-reset mode - current waiters are woken by the version, the flag is left for a next waiter only when nobody waits
			if constexpr (_ManualReset)
			{
				_state.signalized.store(true, std::memory_order_release);
			}
			else
			{
				_state.auto_reset_version.fetch_add(1, std::memory_order_acq_rel);
				if (_state.waiters == 0)
					_state.signalized.store(true, std::memory_order_release);
			}
		}
		_primitive.notify_all();
	}

	void signalize() noexcept
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		std::lock_guard lock(_state.state_mutex);
		_state.signalized.store(true, std::memory_order_release);
		_state.signal_version.fetch_add(1, std::memory_order_acq_rel);
		std::ignore = _primitive.try_acquire();
		_primitive.release();
	}

	void reset() noexcept
		requires _ManualReset
	{
		std::lock_guard lock(_state.state_mutex);
		_state.signalized.store(false, std::memory_order_release);
		_state.manual_reset_version.fetch_add(1, std::memory_order_acq_rel);
	}

	[[nodiscard]] bool is_signalized() const noexcept
	{
		return _state.signalized.load(std::memory_order_acquire);
	}

	[[nodiscard]] uint64_t get_signal_version() const noexcept
	{
		return _state.signal_version.load(std::memory_order_acquire);
	}

	[[nodiscard]] uint64_t get_reset_version() const noexcept
	{
		return _state.manual_reset_version.load(std::memory_order_acquire);
	}

private:
	static constexpr auto _no_predicate = []() noexcept
	{
		return false;
	};

	template <class _TLock, class _PredicateType>
	void _wait(_TLock& lock, _PredicateType&& pred) const
	{
		if (pred())
			return;

		for (;;)
		{
			std::unique_lock state_lock(_state.state_mutex);
			if (pred())
				return;

			lock.unlock();
			++_state.waiters;
			_primitive.wait(state_lock);
			--_state.waiters;
			state_lock.unlock();
			lock.lock();
		}
	}

	template <class _TLock, class _TClock, class _TDuration, class _PredicateType>
	bool _wait_until(_TLock& lock, const std::chrono::time_point<_TClock, _TDuration>& abs_time, _PredicateType&& pred) const
	{
		if (pred())
			return true;

		for (bool timed_out = false;;)
		{
			std::unique_lock state_lock(_state.state_mutex);
			if (pred())
				return true;

			if (timed_out)
				return false;

			lock.unlock();
			++_state.waiters;
			timed_out = _primitive.wait_until(state_lock, abs_time) == std::cv_status::timeout;
			--_state.waiters;
			state_lock.unlock();
			lock.lock();
		}
	}

	uint64_t _get_initial_version() const noexcept
		requires _ManualReset
	{
		return _state.signal_version.load(std::memory_order_acquire);
	}

	uint64_t _get_initial_version() const noexcept
		requires(!_ManualReset)
	{
		return _state.auto_reset_version.load(std::memory_order_acquire);
	}

	bool _check_signal(uint64_t initial_ver) const noexcept
		requires _ManualReset
	{
		if (_state.signal_version.load(std::memory_order_acquire) != initial_ver)
			return true;

		return _state.signalized.load(std::memory_order_acquire);
	}

	bool _check_signal(uint64_t initial_ver) const noexcept
		requires(!_ManualReset)
	{
		if (_state.auto_reset_version.load(std::memory_order_acquire) != initial_ver)
			return true;

		// Auto-reset: atomically check and reset
		return _state.signalized.exchange(false, std::memory_order_acq_rel);
	}

private:
	/// <summary>
	/// state management for persistent signaling
	/// </summary>
	struct signal_state
	{
		mutable std::mutex state_mutex;
		std::atomic<bool> signalized{ false };
		std::atomic<uint64_t> signal_version{ 0 }; // Detects signal changes

		// auto-reset mode - broadcast to all waiters signalize_all()
		std::atomic<uint64_t> auto_reset_version{ 0 };

		// manual-reset mode - wake all waiters
		std::atomic<uint64_t> manual_reset_version{ 0 };

		// condition variable mode - blocked waiters, guarded by state_mutex
		uint64_t waiters = 0;
	};

private:
	mutable _SyncPrimitive _primitive;
	mutable signal_state _state;
};

template <class _Type, class _InnerType = std::binary_semaphore>
concept signal_type = std::is_same_v<_Type, janecekvit::synchronization::signal<_InnerType>>;

} // namespace janecekvit::synchronization
