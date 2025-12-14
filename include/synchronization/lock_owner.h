/*
MIT License
Copyright (c) 2025 Vit Janecek (mailto:janecekvit@outlook.com)

concurrent.h
Purpose:	header file contains set of thread-safe concurrent containers,
			also methods that implemented over basic stl containers and
			thread-safe methods for every possible concurrent object


@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 2.04 9/1/2022
*/

#pragma once
#include "compatibility/compiler_support.h"
#include "extensions/constraints.h"
#include "synchronization/signal.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <concepts>
#include <csignal>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <source_location>
#include <stop_token>
#include <system_error>
#include <thread>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <variant>

/// Namespace owns set of thread-safe concurrent containers and methods that implemented over basic stl containers
///  and thread-safe methods for every possible concurrent object
namespace janecekvit::synchronization
{
/// <summary>
/// Concept for supported mutex types
/// </summary>
template <class _Type>
concept is_supported_mutex = constraints::is_shared_mutex_type<_Type> || constraints::is_mutex_type<_Type>;

/// <summary>
/// Concept for lock tracking policies
/// </summary>
template <typename Policy>
concept lock_tracking_policy = requires {
	{ Policy::is_compile_time } -> std::convertible_to<bool>;
	{ Policy::should_track() } -> std::convertible_to<bool>;
};

/// <summary>
/// Information about a lock acquisition event.
/// Captures mutex type, resource type, source location, thread ID and timestamp.
///
/// </summary>
class lock_information
{
public:
	std::type_index MutexType = typeid(void);
	std::source_location Location;
	std::thread::id ThreadId = std::this_thread::get_id();
	std::chrono::system_clock::time_point AcquiredAt = std::chrono::system_clock::now();

	std::optional<std::type_index> ResourceType;
};

/// <summary>
/// Callback function type for lock event logging.
/// Called on successful lock acquisition with event details and specific mutex.
/// </summary>
using lock_event_callback = std::function<void(
	const lock_information& lock_info,
	const void* mutex_ptr)>;

/// <summary>
/// Base class providing shared logging infrastructure for lock tracking policies.
/// Provides thread-safe callback management and event logging for every lock acquisition.
/// </summary>
class lock_logging_support
{
	template <class, lock_tracking_policy, class>
	friend class lock_holder_base;

public:
	static void set_logging_callback(lock_event_callback&& callback) noexcept
	{
		std::unique_lock lock(_logging_callback_mutex());
		_logging_callback() = std::move(callback);
		_has_logging_callback().store(true, std::memory_order_release);
	}

	static void clear_logging_callback() noexcept
	{
		std::unique_lock lock(_logging_callback_mutex());
		_logging_callback() = nullptr;
		_has_logging_callback().store(false, std::memory_order_release);
	}

private:
	static void _log_event(const lock_information& lock_info, const void* mutex_ptr) noexcept
	{
		if (!_has_logging_callback().load(std::memory_order_acquire))
			return;

		try
		{
			std::shared_lock lock(_logging_callback_mutex());
			if (_logging_callback())
			{
				_logging_callback()(lock_info, mutex_ptr);
			}
		}
		catch (...)
		{
			// Swallow all exceptions from user callback to prevent breaking lock semantics
		}
	}

	static std::shared_mutex& _logging_callback_mutex()
	{
		static std::shared_mutex mutex;
		return mutex;
	}

	static lock_event_callback& _logging_callback()
	{
		static lock_event_callback callback;
		return callback;
	}

	static std::atomic<bool>& _has_logging_callback()
	{
		static std::atomic<bool> has_callback{ false };
		return has_callback;
	}
};

/// <summary>
/// Compile-time policy: tracking disabled
/// </summary>
struct lock_tracking_disabled
{
	static constexpr bool is_compile_time = true;

	static constexpr bool should_track() noexcept
	{
		return false;
	}
};

/// <summary>
/// Compile-time policy: tracking enabled with logging support
/// </summary>
struct lock_tracking_enabled : lock_logging_support
{
	static constexpr bool is_compile_time = true;

	static constexpr bool should_track() noexcept
	{
		return true;
	}
};

/// <summary>
/// Runtime policy: checks custom callback or environment variable to enable/disable tracking at runtime
/// </summary>
struct lock_tracking_runtime : lock_logging_support
{
	static constexpr bool is_compile_time = false;

	static bool should_track() noexcept
	{
		return _get_runtime_decision();
	}

	static void set_callback(std::function<bool()>&& callback)
	{
		auto shared_callback = std::make_shared<std::function<bool()>>(std::move(callback));

		_custom_callback().store(shared_callback, std::memory_order_release);
		_has_custom_callback().store(true, std::memory_order_release);
	}

	static void clear_callback() noexcept
	{
		_custom_callback().store(nullptr, std::memory_order_release);
		_has_custom_callback().store(false, std::memory_order_release);
	}

private:
	static bool _get_runtime_decision() noexcept
	{
		if (!_has_custom_callback().load(std::memory_order_acquire))
			return false;

		auto callback = _custom_callback().load(std::memory_order_acquire);
		if (callback && *callback)
			return (*callback)();

		return false;
	}

	static std::atomic<std::shared_ptr<std::function<bool()>>>& _custom_callback()
	{
		static std::atomic<std::shared_ptr<std::function<bool()>>> callback;
		return callback;
	}

	static std::atomic<bool>& _has_custom_callback()
	{
		static std::atomic<bool> has_callback{ false };
		return has_callback;
	}
};

/// <summary>
/// Forward declaration of lock owner base
/// </summary>
template <is_supported_mutex _MutexType, lock_tracking_policy _Policy = lock_tracking_disabled>
class lock_owner_base;

/// <summary>
/// Base class for lock-holder types with common policy-based tracking utilities.
/// Provides shared helper functions for both exclusive and concurrent lock holders.
/// </summary>
template <class _Type, lock_tracking_policy _Policy, class _LockType>
class lock_holder_base
{
public:
	~lock_holder_base() = default;

	constexpr lock_holder_base(lock_owner_base<_Type, _Policy>& owner, bool tracking_enabled, _LockType&& lock) noexcept
		: _owner(&owner)
		, _tracking_enabled(tracking_enabled)
		, _lock(std::move(lock))
	{
	}

	constexpr lock_holder_base(lock_owner_base<_Type, _Policy>& owner, bool tracking_enabled, _LockType&& lock, std::type_index&& resourceType) noexcept
		requires(!_Policy::is_compile_time || (_Policy::is_compile_time && _Policy::should_track()))
		: lock_holder_base(owner, tracking_enabled, std::move(lock))
	{
		_resourceType = std::move(resourceType);
	}

	constexpr void unlock()
	{
		_check_ownership();
		_lock.unlock();

		if constexpr (_needs_runtime_tracking())
		{
			if (_tracking_enabled)
			{
				_pop_lock_details();
				_tracking_enabled = false;
			}
		}
	}

	constexpr void lock(std::source_location srcl = std::source_location::current())
	{
		_check_deadlock();
		_lock.lock();

		if constexpr (_needs_runtime_tracking())
		{
			if (_should_track())
			{
				const auto& lock_info = _push_lock_details(typeid(_Type), std::move(srcl));
				_tracking_enabled = true;
				_log_lock_event(lock_info);
			}
		}
	}

	constexpr bool try_lock(std::source_location srcl = std::source_location::current())
	{
		_check_deadlock();
		bool locked = _lock.try_lock();

		if constexpr (_needs_runtime_tracking())
		{
			if (locked && _should_track())
			{
				const auto& lock_info = _push_lock_details(typeid(_Type), std::move(srcl));
				_tracking_enabled = true;
				_log_lock_event(lock_info);
			}
		}

		return locked;
	}

	template <class _Condition, class _Predicate>
		requires constraints::condition_variable_type<_Condition>
	constexpr decltype(auto) wait(_Condition& cv, _Predicate&& pred) const
	{
		_check_ownership();
		return cv.wait(_lock, std::move(pred));
	}

	template <class _Condition>
		requires constraints::condition_variable_type<_Condition>
	constexpr decltype(auto) wait(_Condition& cv) const
	{
		_check_ownership();
		return cv.wait(_lock);
	}

	template <constraints::condition_variable_type _Condition, class _TRep, class _TPeriod>
	[[nodiscard]] decltype(auto) wait_for(_Condition& cv, const std::chrono::duration<_TRep, _TPeriod>& rel_time) const
	{
		_check_ownership();
		return cv.wait_for(_lock, rel_time);
	}

	template <constraints::condition_variable_type _Condition, class _TRep, class _TPeriod, class _Predicate>
	[[nodiscard]] decltype(auto) wait_for(_Condition& cv, const std::chrono::duration<_TRep, _TPeriod>& rel_time, _Predicate&& pred) const
	{
		_check_ownership();
		return cv.wait_for(_lock, rel_time, std::move(pred));
	}

#if defined(HAS_JTHREAD)
	template <constraints::condition_variable_type _Condition, class _TRep, class _TPeriod, class _Predicate>
	[[nodiscard]] decltype(auto) wait_for(_Condition& cv, std::stop_token stoken, const std::chrono::duration<_TRep, _TPeriod>& rel_time, _Predicate&& pred) const
	{
		_check_ownership();
		return cv.wait_for(_lock, std::move(stoken), rel_time, std::move(pred));
	}
#endif // HAS_JTHREAD

	constexpr bool owns_lock() const noexcept
	{
		return _lock.owns_lock();
	}

	operator bool() const noexcept
	{
		return owns_lock();
	}

protected:
	constexpr void _check_deadlock() const
	{
		if (_lock.owns_lock())
			throw std::system_error(EDEADLK, std::system_category().default_error_condition(EDEADLK).category(), "lock_holder already owns the resource!");
	}

	constexpr void _check_ownership() const
	{
		if (!_lock.owns_lock())
			throw std::system_error(EPERM, std::system_category().default_error_condition(EPERM).category(), "lock_holder does not own the resource!");
	}

	void _log_lock_event(const lock_information& lock_info) const
	{
		if constexpr (_needs_runtime_tracking())
			_Policy::_log_event(lock_info, _owner->get_mutex().get());
	}

	static constexpr bool _should_track() noexcept
	{
		if constexpr (_Policy::is_compile_time)
			return _Policy::should_track();

		else
			return _Policy::should_track();
	}

	static constexpr bool _needs_runtime_tracking() noexcept
	{
		return !_Policy::is_compile_time || _Policy::should_track();
	}

	const lock_information& _push_lock_details(std::type_index&& index, std::source_location&& srcl)
	{
		if constexpr (std::is_same_v<_LockType, std::unique_lock<_Type>>)
		{
			if constexpr (_needs_runtime_tracking())
			{
				if (_resourceType.has_value())
					return _owner->_push_exclusive_lock_details(std::move(index), std::move(srcl), _resourceType.value());
				else
					return _owner->_push_exclusive_lock_details(std::move(index), std::move(srcl));
			}
		}
		else if constexpr (std::is_same_v<_LockType, std::shared_lock<_Type>>)
		{
			if constexpr (_needs_runtime_tracking())
			{
				if (_resourceType.has_value())
					return _owner->_push_concurrent_lock_details(this, std::move(index), std::move(srcl), _resourceType.value());
				else
					return _owner->_push_concurrent_lock_details(this, std::move(index), std::move(srcl));
			}
		}
	}

	void _pop_lock_details()
	{
		if constexpr (std::is_same_v<_LockType, std::unique_lock<_Type>>)
			_owner->_pop_exclusive_lock_details();

		else if constexpr (std::is_same_v<_LockType, std::shared_lock<_Type>>)
			_owner->_pop_concurrent_lock_details(this);
	}

	lock_owner_base<_Type, _Policy>* _owner = nullptr;
	bool _tracking_enabled;
	mutable _LockType _lock;

	std::conditional_t<!_Policy::is_compile_time || (_Policy::is_compile_time && _Policy::should_track()), std::optional<std::type_index>, std::monostate> _resourceType;
};

/// <summary>
/// RAII holder that acquires and manages an exclusive lock on a mutex owned by a lock_owner_base.
/// </summary>
/// <typeparam name="_Type">The mutex type used by the lock owner (e.g., std::mutex or a compatible mutex type).</typeparam>
/// <typeparam name="_Policy">Lock-tracking policy type that controls whether lock acquisitions are recorded for debugging/tracing. Policy may decide tracking at compile-time or runtime.</typeparam>
template <class _Type, lock_tracking_policy _Policy>
class [[nodiscard]] exclusive_lock_holder : public lock_holder_base<_Type, _Policy, std::unique_lock<_Type>>
{
private:
	using Base = lock_holder_base<_Type, _Policy, std::unique_lock<_Type>>;
	using Base::_lock;
	using Base::_owner;
	using Base::_tracking_enabled;

public:
	constexpr exclusive_lock_holder(lock_owner_base<_Type, _Policy>& owner) noexcept
		: Base(owner, false, std::unique_lock<_Type>(*owner.get_mutex()))
	{
	}

	constexpr exclusive_lock_holder(lock_owner_base<_Type, _Policy>& owner, std::source_location&& srcl) noexcept
		: Base(owner, Base::_should_track(), std::unique_lock<_Type>(*owner.get_mutex()))
	{
		if constexpr (Base::_needs_runtime_tracking())
		{
			if (_tracking_enabled)
			{
				const auto& lock_info = this->_push_lock_details(typeid(_Type), std::move(srcl));
				Base::_log_lock_event(lock_info);
			}
		}
	}

	constexpr exclusive_lock_holder(lock_owner_base<_Type, _Policy>& owner, std::source_location&& srcl, std::type_index&& resourceType) noexcept
		: Base(owner, Base::_should_track(), std::unique_lock<_Type>(*owner.get_mutex()), std::move(resourceType))
	{
		if constexpr (Base::_needs_runtime_tracking())
		{
			if (_tracking_enabled)
			{
				const auto& lock_info = this->_push_lock_details(typeid(_Type), std::move(srcl));
				Base::_log_lock_event(lock_info);
			}
		}
	}

	~exclusive_lock_holder()
	{
		if constexpr (Base::_needs_runtime_tracking())
		{
			if (_tracking_enabled)
				this->_pop_lock_details();
		}
	}

	constexpr exclusive_lock_holder(const exclusive_lock_holder& other) noexcept = delete;

	constexpr exclusive_lock_holder(exclusive_lock_holder&& other) noexcept
		: Base(*other._owner, other._tracking_enabled, std::move(other._lock))
	{
		other._tracking_enabled = false;
	}

	constexpr exclusive_lock_holder& operator=(const exclusive_lock_holder& other) noexcept = delete;

	constexpr exclusive_lock_holder& operator=(exclusive_lock_holder&& other) noexcept
	{
		_owner = std::move(other._owner);
		_lock = std::move(other._lock);
		_tracking_enabled = other._tracking_enabled;
		other._tracking_enabled = false;
		return *this;
	}
};

/// <summary>
/// RAII holder that acquires and manages an shared lock on a mutex owned by a lock_owner_base.
/// </summary>
/// <typeparam name="_Type">The mutex type used by the lock owner (e.g., std::mutex or a compatible mutex type).</typeparam>
/// <typeparam name="_Policy">Lock-tracking policy type that controls whether lock acquisitions are recorded for debugging/tracing. Policy may decide tracking at compile-time or runtime.</typeparam>
template <class _Type, lock_tracking_policy _Policy>
class [[nodiscard]] concurrent_lock_holder : public lock_holder_base<_Type, _Policy, std::shared_lock<_Type>>
{
private:
	using Base = lock_holder_base<_Type, _Policy, std::shared_lock<_Type>>;
	using Base::_lock;
	using Base::_owner;
	using Base::_tracking_enabled;

public:
	constexpr concurrent_lock_holder(lock_owner_base<_Type, _Policy>& owner) noexcept
		: Base(owner, false, std::shared_lock<_Type>(*owner.get_mutex()))
	{
	}

	constexpr concurrent_lock_holder(lock_owner_base<_Type, _Policy>& owner, std::source_location&& srcl) noexcept
		: Base(owner, Base::_should_track(), std::shared_lock<_Type>(*owner.get_mutex()))
	{
		if constexpr (Base::_needs_runtime_tracking())
		{
			if (_tracking_enabled)
			{
				const auto& lock_info = this->_push_lock_details(typeid(_Type), std::move(srcl));
				Base::_log_lock_event(lock_info);
			}
		}
	}

	constexpr concurrent_lock_holder(lock_owner_base<_Type, _Policy>& owner, std::source_location&& srcl, std::type_index&& resourceType) noexcept
		: Base(owner, Base::_should_track(), std::shared_lock<_Type>(*owner.get_mutex()), std::move(resourceType))
	{
		if constexpr (Base::_needs_runtime_tracking())
		{
			if (_tracking_enabled)
			{
				const auto& lock_info = this->_push_lock_details(typeid(_Type), std::move(srcl));
				Base::_log_lock_event(lock_info);
			}
		}
	}

	~concurrent_lock_holder()
	{
		if constexpr (Base::_needs_runtime_tracking())
		{
			if (_tracking_enabled)
				this->_pop_lock_details();
		}
	}

	constexpr concurrent_lock_holder(const concurrent_lock_holder& other) noexcept = delete;

	constexpr concurrent_lock_holder(concurrent_lock_holder&& other) noexcept
		: Base(*other._owner, other._tracking_enabled, std::move(other._lock))
	{
		if constexpr (Base::_needs_runtime_tracking())
		{
			if (_tracking_enabled && _lock.owns_lock())
				_owner->_move_concurrent_lock_details(&other, this);
		}

		other._tracking_enabled = false;
	}

	constexpr concurrent_lock_holder& operator=(const concurrent_lock_holder& other) noexcept = delete;

	constexpr concurrent_lock_holder& operator=(concurrent_lock_holder&& other) noexcept
	{
		_owner = std::move(other._owner);
		_lock = std::move(other._lock);
		_tracking_enabled = other._tracking_enabled;

		if constexpr (Base::_needs_runtime_tracking())
		{
			if (_tracking_enabled && _lock.owns_lock())
				_owner->_move_concurrent_lock_details(&other, this);
		}

		other._tracking_enabled = false;

		return *this;
	}
};

/// <summary>
/// This class is intended to be used by lock holder types to record and query lock acquisition locations and related information.
/// If _MutexType is a shared mutex type (constraints::is_shared_mutex_type), the class maintains per-holder concurrent lock details,
///	otherwise those concurrent-detail APIs are not available.
/// </summary>
/// <typeparam name="_MutexType">The mutex type associated with the owner. Must satisfy the is_supported_mutex constraint.</typeparam>
template <is_supported_mutex _MutexType>
class [[nodiscard]] owner_lock_details
{
	template <class, lock_tracking_policy, class>
	friend class lock_holder_base;

	template <class, lock_tracking_policy>
	friend class exclusive_lock_holder;

	template <class, lock_tracking_policy>
	friend class concurrent_lock_holder;

public:
	using exclusive_lock_details = typename std::optional<lock_information>;
	using concurrent_lock_details = typename std::conditional_t<constraints::is_shared_mutex_type<_MutexType>, std::unordered_map<void*, lock_information>, std::monostate>;
	using mutex_lock_details = typename std::shared_ptr<std::mutex>;

public:
	virtual ~owner_lock_details() = default;

	[[nodiscard]] exclusive_lock_details get_exclusive_lock_details() const noexcept
	{
		return _exclusive_lock_details;
	}

	[[nodiscard]] concurrent_lock_details get_concurrent_lock_details() const noexcept
		requires(constraints::is_shared_mutex_type<_MutexType>)
	{
		return _concurrent_lock_details;
	}

private:
	const lock_information& _push_exclusive_lock_details(std::type_index&& mutexType, std::source_location&& srcl, std::optional<std::type_index> resourceType = {})
	{
		_exclusive_lock_details = lock_information{
			std::move(mutexType),
			std::move(srcl),
			std::this_thread::get_id(),
			std::chrono::system_clock::now(),
			std::move(resourceType),
		};
		return *_exclusive_lock_details;
	}

	void _pop_exclusive_lock_details() noexcept
	{
		_exclusive_lock_details.reset();
	}

	const lock_information& _push_concurrent_lock_details(void* wrapper, std::type_index&& mutexType, std::source_location&& srcl, std::optional<std::type_index> resourceType = {}) const
		requires(constraints::is_shared_mutex_type<_MutexType>)
	{
		std::unique_lock lck(*_mutex_lock_details);
		auto [it, inserted] = _concurrent_lock_details.emplace(
			wrapper,
			lock_information{
				std::move(mutexType),
				std::move(srcl),
				std::this_thread::get_id(),
				std::chrono::system_clock::now(),
				std::move(resourceType) });

		return it->second;
	}

	void _pop_concurrent_lock_details(void* wrapper) const noexcept
		requires(constraints::is_shared_mutex_type<_MutexType>)
	{
		std::unique_lock lck(*_mutex_lock_details);
		_concurrent_lock_details.erase(wrapper);
	}

	void _move_concurrent_lock_details(void* old, void* newone) const
		requires(constraints::is_shared_mutex_type<_MutexType>)
	{
		std::unique_lock lck(*_mutex_lock_details);
		auto&& node = _concurrent_lock_details.extract(old);
		if (node.empty())
			return;

		_concurrent_lock_details.emplace(newone, std::move(node.mapped()));
	}

private:
	mutable mutex_lock_details _mutex_lock_details = std::make_shared<std::mutex>();
	mutable exclusive_lock_details _exclusive_lock_details;

	mutable concurrent_lock_details _concurrent_lock_details;
};

template <is_supported_mutex _Type, lock_tracking_policy _Policy>
class [[nodiscard]] lock_owner_base : public std::conditional_t<_Policy::is_compile_time && !_Policy::should_track(), std::monostate, owner_lock_details<_Type>>
{
	friend exclusive_lock_holder<_Type, _Policy>;
	friend concurrent_lock_holder<_Type, _Policy>;

public:
	using policy_type = _Policy;
	using exclusive_holder_type = exclusive_lock_holder<_Type, _Policy>;
	using concurrent_holder_type = concurrent_lock_holder<_Type, _Policy>;

public:
	constexpr lock_owner_base() = default;

	constexpr lock_owner_base(_Type&& object) noexcept
		: _mutex(std::make_shared<_Type>(std::forward<_Type>(object)))
	{
	}

	~lock_owner_base()
	{
		// For compile-time tracking
		if constexpr (_Policy::is_compile_time && _Policy::should_track())
		{
			// Ensure that no operations are running on resource while destruction is in the process
			// -> Prevent this undefined behavior create deadlock in debugging mode
			exclusive_lock_holder<_Type, _Policy> cleanup(*this, std::source_location::current());
		}
		// For runtime tracking
		else if constexpr (!_Policy::is_compile_time)
		{
			if (_Policy::should_track())
			{
				// Ensure that no operations are running on resource while destruction is in the process
				// -> Prevent this undefined behavior create deadlock in debugging mode
				exclusive_lock_holder<_Type, _Policy> cleanup(*this, std::source_location::current());
			}
		}
	}

	// For compile-time disabled tracking
	[[nodiscard]] constexpr auto exclusive() noexcept
		requires(_Policy::is_compile_time && !_Policy::should_track())
	{
		return exclusive_lock_holder<_Type, _Policy>(*this);
	}

	// For compile-time enabled tracking
	[[nodiscard]] constexpr auto exclusive(std::source_location srcl = std::source_location::current()) noexcept
		requires(_Policy::is_compile_time && _Policy::should_track())
	{
		return exclusive_lock_holder<_Type, _Policy>(*this, std::move(srcl));
	}

	// For runtime tracking
	[[nodiscard]] auto exclusive(std::source_location srcl = std::source_location::current()) noexcept
		requires(!_Policy::is_compile_time)
	{
		if (_Policy::should_track())
			return exclusive_lock_holder<_Type, _Policy>(*this, std::move(srcl));

		return exclusive_lock_holder<_Type, _Policy>(*this);
	}

	// For compile-time disabled tracking
	[[nodiscard]] constexpr auto concurrent() const noexcept
		requires(_Policy::is_compile_time && !_Policy::should_track() && constraints::is_shared_mutex_type<_Type>)
	{
		return concurrent_lock_holder<_Type, _Policy>(const_cast<lock_owner_base<_Type, _Policy>&>(*this));
	}

	// For compile-time enabled tracking
	[[nodiscard]] constexpr auto concurrent(std::source_location srcl = std::source_location::current()) const noexcept
		requires(_Policy::is_compile_time && _Policy::should_track() && constraints::is_shared_mutex_type<_Type>)
	{
		return concurrent_lock_holder<_Type, _Policy>(const_cast<lock_owner_base<_Type, _Policy>&>(*this), std::move(srcl));
	}

	// For runtime tracking
	[[nodiscard]] auto concurrent(std::source_location srcl = std::source_location::current()) const noexcept
		requires(!_Policy::is_compile_time && constraints::is_shared_mutex_type<_Type>)
	{
		if (_Policy::should_track())
			return concurrent_lock_holder<_Type, _Policy>(const_cast<lock_owner_base<_Type, _Policy>&>(*this), std::move(srcl));

		return concurrent_lock_holder<_Type, _Policy>(const_cast<lock_owner_base<_Type, _Policy>&>(*this));
	}

	[[nodiscard]] const std::shared_ptr<_Type> get_mutex() const noexcept
	{
		return _mutex;
	}

private:
	std::shared_ptr<_Type> _mutex = std::make_shared<_Type>();
};

// CTAD
template <class T>
lock_owner_base(T&&) -> lock_owner_base<T, lock_tracking_disabled>;

template <class T>
lock_owner_base(T&&) -> lock_owner_base<T, lock_tracking_enabled>;

/// <summary>
/// Compile-time aliases for lock tracking
/// </summary>
template <class _Type = std::shared_mutex>
using lock_owner_release = lock_owner_base<_Type, lock_tracking_disabled>;

template <class _Type = std::shared_mutex>
using lock_owner_debug = lock_owner_base<_Type, lock_tracking_enabled>;

/// <summary>
/// Runtime alias for lock tracking
/// </summary>
template <class _Type = std::shared_mutex>
using lock_owner_runtime = lock_owner_base<_Type, lock_tracking_runtime>;

/// <summary>
/// Build configuration alias
/// </summary>
#if defined(NDEBUG) || defined(SYNCHRONIZATION_NO_TRACKING)
template <class _Type = std::shared_mutex>
using result_lock_owner = lock_owner_release<_Type>;
#elif defined(SYNCHRONIZATION_RUNTIME_TRACKING)
template <class _Type = std::shared_mutex>
using result_lock_owner = lock_owner_runtime<_Type>;
#else
template <class _Type = std::shared_mutex>
using lock_owner = lock_owner_debug<_Type>;
#endif // _DEBUG

} // namespace janecekvit::synchronization
