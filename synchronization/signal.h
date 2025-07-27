#pragma once
#include "extensions/constraints.h"
#include "extensions/finally.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
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
		const auto initial_reset_gen = _state.reset_generation.load(std::memory_order_acquire);

		_primitive.wait(lock, [this, initial_reset_gen]() -> bool
			{
				// Check for hard reset first
				if (_state.reset_generation.load(std::memory_order_acquire) != initial_reset_gen)
				{
					return true; // Wake up due to hard reset
				}

				return _reset_strategy();
			});
	}

	template <class _TLock, class _PredicateType>
	void wait(_TLock& lock, _PredicateType&& pred) const
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive> && std::predicate<_PredicateType>
#endif
	{
		const auto initial_reset_gen = _state.reset_generation.load(std::memory_order_acquire);

		_primitive.wait(lock, [this, initial_reset_gen, predicate = std::move(pred)]() -> bool
			{
				// Check for hard reset first
				if (_state.reset_generation.load(std::memory_order_acquire) != initial_reset_gen)
				{
					return true;
				}

				return _reset_strategy() || predicate();
			});
	}

#ifdef __cpp_lib_semaphore
	void wait() const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		for (; !_reset_strategy();)
			_primitive.acquire();
	}

	template <class _PredicateType>
	void wait(_PredicateType&& pred) const
		requires constraints::semaphore_type<_SyncPrimitive, 1> && std::predicate<_PredicateType>
	{
		for (; !(_reset_strategy() || pred());)
			_primitive.acquire();
	}
#endif

	template <class _TLock, class TRep, class TPeriod>
	[[nodiscard]] bool wait_for(_TLock& lock, const std::chrono::duration<TRep, TPeriod>& rel_time) const
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
		const auto initial_reset_gen = _state.reset_generation.load(std::memory_order_acquire);
		const auto deadline = std::chrono::steady_clock::now() + rel_time;

		return _primitive.wait_until(lock, deadline, [this, initial_reset_gen]() -> bool
			{
				if (_state.reset_generation.load(std::memory_order_acquire) != initial_reset_gen)
				{
					return true;
				}
				return _reset_strategy();
			});
	}

	template <class _TLock, class TRep, class TPeriod, class _PredicateType>
	[[nodiscard]] bool wait_for(_TLock& lock, const std::chrono::duration<TRep, TPeriod>& rel_time, _PredicateType&& pred) const
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive> && std::predicate<_PredicateType>
#endif
	{
		const auto initial_reset_gen = _state.reset_generation.load(std::memory_order_acquire);
		const auto deadline = std::chrono::steady_clock::now() + rel_time;

		return _primitive.wait_until(lock, deadline, [this, initial_reset_gen, predicate = std::move(pred)]() -> bool
			{
				if (_state.reset_generation.load(std::memory_order_acquire) != initial_reset_gen)
				{
					return true;
				}
				return _reset_strategy() || predicate();
			});
	}

#ifdef __cpp_lib_semaphore
	template <class TRep, class TPeriod>
	[[nodiscard]] bool wait_for(const std::chrono::duration<TRep, TPeriod>& rel_time) const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		const auto deadline = std::chrono::steady_clock::now() + rel_time;

		while (!_reset_strategy())
		{
			if (!_primitive.try_acquire_until(deadline))
				return _reset_strategy();
		}

		return true;
	}

	template <class TRep, class TPeriod, class _PredicateType>
	[[nodiscard]] bool wait_for(const std::chrono::duration<TRep, TPeriod>& rel_time, _PredicateType&& pred) const
		requires constraints::semaphore_type<_SyncPrimitive, 1> && std::predicate<_PredicateType>
	{
		const auto deadline = std::chrono::steady_clock::now() + rel_time;
		auto predicate = std::move(pred);
		for (; !(_reset_strategy() || predicate());)
		{
			if (!_primitive.try_acquire_until(deadline))
				return _reset_strategy() || predicate();
		}
		return true;
	}
#endif

	template <class _TLock, class _TClock, class _TDuration>
	[[nodiscard]] bool wait_until(_TLock& lock, const std::chrono::time_point<_TClock, _TDuration>& abs_time) const
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
		const auto initial_reset_gen = _state.reset_generation.load(std::memory_order_acquire);

		return _primitive.wait_until(lock, abs_time, [this, initial_reset_gen]() -> bool
			{
				if (_state.reset_generation.load(std::memory_order_acquire) != initial_reset_gen)
				{
					return true;
				}
				return _reset_strategy();
			});
	}

	template <class _TLock, class _TClock, class _TDuration, class _PredicateType>
	[[nodiscard]] bool wait_until(_TLock& lock, const std::chrono::time_point<_TClock, _TDuration>& abs_time, _PredicateType&& pred) const
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive> && std::predicate<_PredicateType>
#endif
	{
		const auto initial_reset_gen = _state.reset_generation.load(std::memory_order_acquire);

		return _primitive.wait_until(lock, abs_time, [this, initial_reset_gen, predicate = std::move(pred)]() -> bool
			{
				if (_state.reset_generation.load(std::memory_order_acquire) != initial_reset_gen)
				{
					return true;
				}
				return _reset_strategy() || predicate();
			});
	}

#ifdef __cpp_lib_semaphore
	template <class _TClock, class _TDuration>
	[[nodiscard]] bool wait_until(const std::chrono::time_point<_TClock, _TDuration>& abs_time) const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		while (!_reset_strategy())
		{
			if (!_primitive.try_acquire_until(abs_time))
			{
				return _reset_strategy();
			}
		}
		return true;
	}

	template <class _TClock, class _TDuration, class _PredicateType>
	[[nodiscard]] bool wait_until(const std::chrono::time_point<_TClock, _TDuration>& abs_time, _PredicateType&& pred) const
		requires constraints::semaphore_type<_SyncPrimitive, 1> && std::predicate<_PredicateType>
	{
		auto predicate = std::move(pred);
		while (!(_reset_strategy() || predicate()))
		{
			if (!_primitive.try_acquire_until(abs_time))
			{
				return _reset_strategy() || predicate();
			}
		}
		return true;
	}
#endif

	void signalize() noexcept
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	{
		{
			std::lock_guard lock(_state.state_mutex);
			_state.signalized.store(true, std::memory_order_release);
			_state.signal_generation.fetch_add(1, std::memory_order_acq_rel);
		}

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
		{
			std::lock_guard lock(_state.state_mutex);
			_state.signalized.store(true, std::memory_order_release);
			_state.signal_generation.fetch_add(1, std::memory_order_acq_rel);
		}
		_primitive.notify_all();
	}

#ifdef __cpp_lib_semaphore
	void signalize() noexcept
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		_state.signalized.store(true, std::memory_order_release);
		_state.signal_generation.fetch_add(1, std::memory_order_acq_rel);
		_primitive.release();
	}
#endif

	void reset() noexcept
#ifdef __cpp_lib_concepts
		requires _ManualReset
#endif
	{
		{
			std::lock_guard lock(_state.state_mutex);
			_state.signalized.store(false, std::memory_order_release);
			_state.hard_reset_requested.store(true, std::memory_order_release);
			_state.reset_generation.fetch_add(1, std::memory_order_acq_rel);
		}

#ifdef __cpp_lib_concepts
		if constexpr (constraints::condition_variable_type<_SyncPrimitive>)
		{
			_primitive.notify_all();
		}
#else
		_primitive.notify_all();
#endif

		// Clear hard reset flag after a brief moment
		_state.hard_reset_requested.store(false, std::memory_order_release);
	}

	[[nodiscard]] bool is_signalized() const noexcept
	{
		return _state.signalized.load(std::memory_order_acquire);
	}

	[[nodiscard]] uint64_t get_signal_generation() const noexcept
	{
		return _state.signal_generation.load(std::memory_order_acquire);
	}

private:
	bool _reset_strategy() const noexcept
	{
		if constexpr (_ManualReset)
		{
			return _state.signalized.load(std::memory_order_acquire);
		}
		else
		{
			// Auto-reset: atomically check and reset
			return _state.signalized.exchange(false, std::memory_order_acq_rel);
		}
	}

private:
	// state management for persistent signaling
	struct signal_state
	{
		std::atomic<bool> signalized{ false };
		std::atomic<uint64_t> signal_generation{ 0 }; // Detects signal changes
		mutable std::mutex state_mutex;

		// For hard reset - wake all waiters
		std::atomic<bool> hard_reset_requested{ false };
		std::atomic<uint64_t> reset_generation{ 0 };
	};

private:
	mutable _SyncPrimitive _primitive;
	mutable signal_state _state;
};

#ifdef __cpp_lib_concepts
template <class _Type, class _InnerType = std::binary_semaphore>
concept signal_type = std::is_same_v<_Type, janecekvit::synchronization::signal<_InnerType>>;
#endif

} // namespace janecekvit::synchronization
