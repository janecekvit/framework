#pragma once
#include "extensions/constraints.h"
#include "synchronization/concurrent.h"
#include "synchronization/signal.h"

#include <atomic>
#include <condition_variable>
#include <unordered_set>

namespace janecekvit::synchronization
{

template <constraints::enum_type _Enum, class _SyncPrimitive = std::binary_semaphore>
	requires constraints::semaphore_type<_SyncPrimitive, 1> || constraints::condition_variable_type<_SyncPrimitive>
class wait_for_multiple_signals
{
public:
	wait_for_multiple_signals() = default;
	virtual ~wait_for_multiple_signals() = default;

	template <class _TLock>
		requires constraints::condition_variable_type<_SyncPrimitive>
	[[nodiscard]] _Enum wait(_TLock& lock) const
	{
		_primitive.wait(lock);
		return _get_state();
	}

	template <class _TLock, class _PredicateType>
		requires constraints::condition_variable_type<_SyncPrimitive> && std::predicate<_PredicateType>
	[[nodiscard]] _Enum wait(_TLock& lock, _PredicateType&& pred) const
	{
		_primitive.wait(lock, std::move(pred));
		return _get_state();
	}

	[[nodiscard]] _Enum wait() const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		_primitive.wait();
		return _get_state();
	}

	template <class _PredicateType>
	[[nodiscard]] _Enum wait(_PredicateType&& pred) const
		requires constraints::semaphore_type<_SyncPrimitive, 1> && std::predicate<_PredicateType>
	{
		_primitive.wait(std::move(pred));
		return _get_state();
	}

	template <class _TLock, class _TRep, class _TPeriod>
		requires constraints::condition_variable_type<_SyncPrimitive>
	[[nodiscard]] std::optional<_Enum> wait_for(_TLock& lock, const std::chrono::duration<_TRep, _TPeriod>& rel_time) const
	{
		if (_primitive.wait_for(lock, rel_time))
			return _get_state();
		return {};
	}

	template <class _TLock, class _TRep, class _TPeriod, class _PredicateType>
		requires constraints::condition_variable_type<_SyncPrimitive> && std::predicate<_PredicateType>
	[[nodiscard]] std::optional<_Enum> wait_for(_TLock& lock, const std::chrono::duration<_TRep, _TPeriod>& rel_time, _PredicateType&& pred) const
	{
		if (_primitive.wait_for(lock, rel_time, std::move(pred)))
			return _get_state();
		return {};
	}

	template <class _TRep, class _TPeriod>
	[[nodiscard]] std::optional<_Enum> wait_for(const std::chrono::duration<_TRep, _TPeriod>& rel_time) const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		if (_primitive.wait_for(rel_time))
			return _get_state();
		return {};
	}

	template <class _TRep, class TPeriod, class _PredicateType>
	[[nodiscard]] std::optional<_Enum> wait_for(const std::chrono::duration<_TRep, TPeriod>& rel_time, _PredicateType&& pred) const
		requires constraints::semaphore_type<_SyncPrimitive, 1> && std::predicate<_PredicateType>
	{
		if (_primitive.wait_for(rel_time, std::move(pred)))
			return _get_state();
		return {};
	}

	template <class _TLock, class _TClock, class _TDuration>
		requires constraints::condition_variable_type<_SyncPrimitive>
	[[nodiscard]] std::optional<_Enum> wait_until(_TLock& lock, const std::chrono::time_point<_TClock, _TDuration>& timeout_time) const
	{
		if (_primitive.wait_until(lock, timeout_time))
			return _get_state();
		return {};
	}

	template <class _TLock, class _TClock, class _TDuration, class _PredicateType>
		requires constraints::condition_variable_type<_SyncPrimitive> && std::predicate<_PredicateType>
	[[nodiscard]] std::optional<_Enum> wait_until(_TLock& lock, const std::chrono::time_point<_TClock, _TDuration>& timeout_time, _PredicateType&& pred) const
	{
		if (_primitive.wait_until(lock, timeout_time, std::move(pred)))
			return _get_state();
		return {};
	}

	template <class _TClock, class _TDuration>
	[[nodiscard]] std::optional<_Enum> wait_until(const std::chrono::time_point<_TClock, _TDuration>& timeout_time) const
		requires constraints::semaphore_type<_SyncPrimitive, 1>
	{
		if (_primitive.wait_until(timeout_time))
			return _get_state();
		return {};
	}

	template <class _TClock, class _TDuration, class _PredicateType>
	[[nodiscard]] std::optional<_Enum> wait_until(const std::chrono::time_point<_TClock, _TDuration>& timeout_time, _PredicateType&& pred) const
		requires constraints::semaphore_type<_SyncPrimitive, 1> && std::predicate<_PredicateType>
	{
		if (_primitive.wait_until(timeout_time, std::move(pred)))
			return _get_state();
		return {};
	}

	void signalize(_Enum state) noexcept
	{
		_state = static_cast<std::underlying_type_t<_Enum>>(state);
		_primitive.signalize();
	}

private:
	_Enum _get_state() const noexcept
	{
		return static_cast<_Enum>(_state.load());
	}

private:
	mutable signal<_SyncPrimitive> _primitive;
	mutable std::atomic<std::underlying_type_t<_Enum>> _state;
};

} // namespace janecekvit::synchronization
