#pragma once
#include "extensions/constraints.h"
#include "synchronization/concurrent.h"
#include "synchronization/signal.h"

#include <atomic>
#include <condition_variable>
#include <unordered_set>

namespace janecekvit::synchronization
{

#if defined(__cpp_lib_concepts) && defined(__cpp_lib_semaphore)
template <constraints::enum_type _Enum, class _SyncPrimitive = std::binary_semaphore>
	requires constraints::semaphore_type<_SyncPrimitive, 1> || constraints::condition_variable_type<_SyncPrimitive>
#else
template <class _Enum, class _SyncPrimitive = std::condition_variable_any>
#endif
class wait_for_multiple_signals
{
private:
	using _Predicate = typename std::function<bool()>;

public:
	wait_for_multiple_signals()			 = default;
	virtual ~wait_for_multiple_signals() = default;

	template <class TLock>
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	[[nodiscard]] _Enum wait(TLock& lock) const
	{
		_syncPrimitive.wait(lock);
		return _get_state();
	}

	template <class TLock>
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	[[nodiscard]] _Enum wait(TLock& lock, _Predicate&& pred) const
	{
		_syncPrimitive.wait(lock, std::move(pred));
		return _get_state();
	}

#ifdef __cpp_lib_semaphore
	[[nodiscard]] _Enum wait() const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		_syncPrimitive.wait();
		return _get_state();
	}
#endif

#ifdef __cpp_lib_semaphore
	[[nodiscard]] _Enum wait(_Predicate&& pred) const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		_syncPrimitive.wait(std::move(pred));
		return _get_state();
	}
#endif

	template <class TLock, class TRep, class TPeriod>
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	[[nodiscard]] std::optional<_Enum> wait_for(TLock& lock, const std::chrono::duration<TRep, TPeriod>& rel_time, std::optional<_Predicate>&& pred = {}) const
	{
		if (_syncPrimitive.wait_for(lock, rel_time, std::move(pred)))
			return _get_state();
		return {};
	}

#ifdef __cpp_lib_semaphore
	template <class TRep, class TPeriod>
	[[nodiscard]] std::optional<_Enum> wait_for(const std::chrono::duration<TRep, TPeriod>& rel_time, std::optional<_Predicate>&& pred = {}) const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		if (_syncPrimitive.wait_for(rel_time, std::move(pred)))
			return _get_state();
		return {};
	}
#endif

	template <class TLock, class TClock, class TDuration>
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_SyncPrimitive>
#endif
	[[nodiscard]] std::optional<_Enum> wait_until(TLock& lock, const std::chrono::time_point<TClock, TDuration>& timeout_time, std::optional<_Predicate>&& pred = {}) const
	{
		if (_syncPrimitive.wait_until(lock, timeout_time, std::move(pred)))
			return _get_state();
		return {};
	}

#ifdef __cpp_lib_semaphore
	template <class TClock, class TDuration>
	[[nodiscard]] std::optional<_Enum> wait_until(const std::chrono::time_point<TClock, TDuration>& timeout_time, std::optional<_Predicate>&& pred = {}) const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		if (_syncPrimitive.wait_until(timeout_time, std::move(pred)))
			return _get_state();
		return {};
	}
#endif

	void signalize(_Enum state) noexcept
	{
		_state = static_cast<std::underlying_type_t<_Enum>>(state);
		_syncPrimitive.signalize();
	}

private:
	_Enum _get_state() const noexcept
	{
		return static_cast<_Enum>(_state.load());
	}

private:
	mutable signal<_SyncPrimitive> _syncPrimitive;
	mutable std::atomic<std::underlying_type_t<_Enum>> _state;
};

} // namespace janecekvit::synchronization