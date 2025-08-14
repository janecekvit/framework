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

#include <array>
#include <cerrno>
#include <chrono>
#include <deque>
#include <exception>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <shared_mutex>
#include <source_location>
#include <stack>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/// Namespace owns set of thread-safe concurrent containers and methods that implemented over basic stl containers
///  and thread-safe methods for every possible concurrent object
namespace janecekvit::synchronization
{
template <class _Type>
concept is_supported_mutex = constraints::is_shared_mutex_type<_Type> || constraints::is_mutex_type<_Type>;

template <is_supported_mutex _MutexType, bool = true>
class lock_owner_base;

template <class _Type, bool _Release>
class [[nodiscard]] exclusive_lock_holder
{
public:
	constexpr exclusive_lock_holder(lock_owner_base<_Type, _Release>& owner) noexcept
		: _owner(&owner)
		, _unique_lock(std::unique_lock<_Type>(*owner.get_mutex()))
	{
	}

	constexpr exclusive_lock_holder(lock_owner_base<_Type, _Release>& owner, std::source_location&& srcl) noexcept
		: _owner(&owner)
		, _unique_lock(std::unique_lock<_Type>(*owner.get_mutex()))
	{
		_owner->_push_exclusive_lock_details(std::move(srcl));
	}

	virtual ~exclusive_lock_holder()
	{
		if constexpr (!_Release)
			_owner->_pop_exclusive_lock_details();
	}

	constexpr exclusive_lock_holder(const exclusive_lock_holder& other) noexcept = delete;

	constexpr exclusive_lock_holder(exclusive_lock_holder&& other) noexcept
		: _owner(other._owner)
		, _unique_lock(std::move(other._unique_lock))
	{
	}

	constexpr exclusive_lock_holder& operator=(const exclusive_lock_holder& other) noexcept = delete;

	constexpr exclusive_lock_holder& operator=(exclusive_lock_holder&& other) noexcept
	{
		_owner = std::move(other._owner);
		_unique_lock = std::move(other._unique_lock);
		return *this;
	}

	constexpr void unlock()
	{
		_check_ownership();
		_unique_lock.unlock();

		if constexpr (!_Release)
			_owner->_pop_exclusive_lock_details();
	}

	constexpr void lock(std::source_location srcl = std::source_location::current())
	{
		_check_deadlock();
		_unique_lock.lock();
		if constexpr (!_Release)
			_owner->_push_exclusive_lock_details(std::move(srcl));
	}

	constexpr bool try_lock(std::source_location srcl = std::source_location::current())
	{
		_check_deadlock();
		bool locked = _unique_lock.try_lock();
		if constexpr (!_Release)
		{
			if (locked)
				_owner->_push_exclusive_lock_details(std::move(srcl));
		}

		return locked;
	}

	template <class _Condition, class _Predicate>
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_Condition>
#endif
	constexpr decltype(auto) wait(_Condition& cv, _Predicate&& pred) const
	{
		_check_ownership();
		return cv.wait(_unique_lock, std::move(pred));
	}

	template <class _Condition>
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_Condition>
#endif
	constexpr decltype(auto) wait(_Condition& cv) const
	{
		_check_ownership();
		return cv.wait(_unique_lock);
	}

	template <constraints::condition_variable_type _Condition, class _TRep, class _TPeriod>
	[[nodiscard]] decltype(auto) wait_for(_Condition& cv, const std::chrono::duration<_TRep, _TPeriod>& rel_time) const
	{
		_check_ownership();
		return cv.wait_for(_unique_lock, rel_time);
	}

	template <constraints::condition_variable_type _Condition, class _TRep, class _TPeriod, class _Predicate>
	[[nodiscard]] decltype(auto) wait_for(_Condition& cv, const std::chrono::duration<_TRep, _TPeriod>& rel_time, _Predicate&& pred) const
	{
		_check_ownership();
		return cv.wait_for(_unique_lock, rel_time, std::move(pred));
	}

#if defined(HAS_JTHREAD)
	template <constraints::condition_variable_type _Condition, class _TRep, class _TPeriod, class _Predicate>
	[[nodiscard]] decltype(auto) wait_for(_Condition& cv, std::stop_token stoken, const std::chrono::duration<_TRep, _TPeriod>& rel_time, _Predicate&& pred) const
	{
		_check_ownership();
		return cv.wait_for(_unique_lock, std::move(stoken), rel_time, std::move(pred));
	}
#endif // HAS_JTHREAD

	constexpr bool owns_lock() const noexcept
	{
		return _unique_lock.owns_lock();
	}

	operator bool() const noexcept
	{
		return owns_lock();
	}

protected:
	constexpr void _check_deadlock() const
	{
		if (_unique_lock.owns_lock())
			throw std::system_error(EDEADLK, std::system_category().default_error_condition(EDEADLK).category(), "exclusive_lock_holder already owns the resource!");
	}

	constexpr void _check_ownership() const
	{
		if (!_unique_lock.owns_lock())
			throw std::system_error(EPERM, std::system_category().default_error_condition(EPERM).category(), "exclusive_lock_holder do not owns the resource!");
	}

private:
	lock_owner_base<_Type, _Release>* _owner = nullptr;
	mutable std::unique_lock<_Type> _unique_lock;
};

template <class _Type, bool _Release>
class [[nodiscard]] concurrent_lock_holder
{
public:
	constexpr concurrent_lock_holder(lock_owner_base<_Type, _Release>& owner) noexcept
		: _owner(&owner)
		, _shared_lock(std::shared_lock<_Type>(*owner.get_mutex()))
	{
	}

	constexpr concurrent_lock_holder(lock_owner_base<_Type, _Release>& owner, std::source_location&& srcl) noexcept
		: _owner(&owner)
		, _shared_lock(std::shared_lock<_Type>(*owner.get_mutex()))
	{
		_owner->_push_concurrent_lock_details(this, std::move(srcl));
	}

	virtual ~concurrent_lock_holder()
	{
		if constexpr (!_Release)
			_owner->_pop_concurrent_lock_details(this);
	}

	constexpr concurrent_lock_holder(const concurrent_lock_holder& other) noexcept = delete;

	constexpr concurrent_lock_holder(concurrent_lock_holder&& other) noexcept
		: _owner(other._owner)
		, _shared_lock(std::move(other._shared_lock))
	{
		if constexpr (!_Release)
		{
			if (_shared_lock.owns_lock())
				_owner->_move_concurrent_lock_details(&other, this);
		}
	}

	constexpr concurrent_lock_holder& operator=(const concurrent_lock_holder& other) noexcept = delete;

	constexpr concurrent_lock_holder& operator=(concurrent_lock_holder&& other) noexcept
	{
		_owner = std::move(other._owner);
		_shared_lock = std::move(other._shared_lock);

		if constexpr (!_Release)
		{
			if (_shared_lock.owns_lock())
				_owner->_move_concurrent_lock_details(&other, this);
		}

		return *this;
	}

	constexpr void unlock()
	{
		_check_ownership();
		_shared_lock.unlock();
		if constexpr (!_Release)
			_owner->_pop_concurrent_lock_details(this);
	}

	constexpr void lock(std::source_location srcl = std::source_location::current())
	{
		_check_deadlock();
		_shared_lock.lock();

		if constexpr (!_Release)
			_owner->_push_concurrent_lock_details(this, std::move(srcl));
	}

	constexpr bool try_lock(std::source_location srcl = std::source_location::current())
	{
		_check_deadlock();
		bool locked = _shared_lock.try_lock();
		if constexpr (!_Release)
		{
			if (locked)
				_owner->_push_concurrent_lock_details(this, std::move(srcl));
		}

		return locked;
	}

	template <class _Condition, class _Predicate>
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_Condition>
#endif
	constexpr decltype(auto) wait(_Condition& cv, _Predicate&& pred) const
	{
		_check_ownership();
		return cv.wait(_shared_lock, std::move(pred));
	}

	template <class _Condition>
#ifdef __cpp_lib_concepts
		requires constraints::condition_variable_type<_Condition>
#endif
	constexpr decltype(auto) wait(_Condition& cv) const
	{
		_check_ownership();
		return cv.wait(_shared_lock);
	}

	template <class _Signal>
#ifdef __cpp_lib_concepts
		requires synchronization::signal_type<_Signal>
#endif
	constexpr decltype(auto) wait(_Signal& cv) const
	{
		_check_ownership();
		return cv.wait(_shared_lock);
	}

	template <constraints::condition_variable_type _Condition, class _TRep, class _TPeriod>
	[[nodiscard]] decltype(auto) wait_for(_Condition& cv, const std::chrono::duration<_TRep, _TPeriod>& rel_time) const
	{
		_check_ownership();
		return cv.wait_for(_shared_lock, rel_time);
	}

	template <constraints::condition_variable_type _Condition, class _TRep, class _TPeriod, class _Predicate>
	[[nodiscard]] decltype(auto) wait_for(_Condition& cv, const std::chrono::duration<_TRep, _TPeriod>& rel_time, _Predicate&& pred) const
	{
		_check_ownership();
		return cv.wait_for(_shared_lock, rel_time, std::move(pred));
	}

#if defined(HAS_JTHREAD)
	template <constraints::condition_variable_type _Condition, class _TRep, class _TPeriod, class _Predicate>
	[[nodiscard]] decltype(auto) wait_for(_Condition& cv, std::stop_token stoken, const std::chrono::duration<_TRep, _TPeriod>& rel_time, _Predicate&& pred) const
	{
		_check_ownership();
		return cv.wait_for(_shared_lock, std::move(stoken), rel_time, std::move(pred));
	}
#endif // HAS_JTHREAD

	constexpr bool owns_lock() const noexcept
	{
		return _shared_lock.owns_lock();
	}

	operator bool() const noexcept
	{
		return owns_lock();
	}

protected:
	constexpr void _check_deadlock() const
	{
		if (_shared_lock.owns_lock())
			throw std::system_error(EDEADLK, std::system_category().default_error_condition(EDEADLK).category(), "exclusive_lock_holder already owns the resource!");
	}

	constexpr void _check_ownership() const
	{
		if (!_shared_lock.owns_lock())
			throw std::system_error(EPERM, std::system_category().default_error_condition(EPERM).category(), "exclusive_lock_holder do not owns the resource!");
	}

private:
	lock_owner_base<_Type, _Release>* _owner = nullptr;
	mutable std::shared_lock<_Type> _shared_lock;
};

class owner_lock_details_release
{
};

class lock_information
{
public:
	std::source_location Location;
	std::thread::id ThreadId = std::this_thread::get_id();
	std::chrono::system_clock::time_point AcquiredAt = std::chrono::system_clock::now();
};

template <is_supported_mutex _MutexType>
class [[nodiscard]] owner_lock_details
{
	friend exclusive_lock_holder<_MutexType, false>;
	friend concurrent_lock_holder<_MutexType, false>;

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
	void _push_exclusive_lock_details(std::source_location&& srcl)
	{
		_exclusive_lock_details = lock_information{ std::move(srcl) };
	}

	void _pop_exclusive_lock_details() noexcept
	{
		_exclusive_lock_details.reset();
	}

	void _push_concurrent_lock_details(void* wrapper, std::source_location&& srcl) const
		requires(constraints::is_shared_mutex_type<_MutexType>)
	{
		std::unique_lock lck(*_mutex_lock_details);
		_concurrent_lock_details.emplace(wrapper, lock_information{ std::move(srcl) });
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

template <is_supported_mutex _Type, bool _Release>
class [[nodiscard]] lock_owner_base : public std::conditional_t<!_Release, owner_lock_details<_Type>, owner_lock_details_release>
{
	friend exclusive_lock_holder<_Type, _Release>;
	friend concurrent_lock_holder<_Type, _Release>;

public:
	using exclusive_holder_type = exclusive_lock_holder<_Type, _Release>;
	using concurrent_holder_type = concurrent_lock_holder<_Type, _Release>;

public:
	constexpr lock_owner_base() = default;

	constexpr lock_owner_base(_Type&& object) noexcept
		: _mutex(std::make_shared<_Type>(std::forward<_Type>(object)))
	{
	}

	~lock_owner_base()
	{
		if constexpr (!_Release)
		{
			// ensure that no operations are running on resource while destruction is in the process
			//->prevent this undefined behavior create deadlock in debugging mode
			exclusive_lock_holder<_Type, _Release> cleanup(*this, std::source_location::current());
		}
	}

	[[nodiscard]] constexpr exclusive_lock_holder<_Type, true> exclusive() noexcept
		requires(_Release)
	{
		return exclusive_lock_holder<_Type, true>(*this);
	}

	[[nodiscard]] constexpr concurrent_lock_holder<_Type, true> concurrent() const noexcept
		requires(_Release && constraints::is_shared_mutex_type<_Type>)
	{
		return concurrent_lock_holder<_Type, true>(const_cast<lock_owner_base<_Type, _Release>&>(*this));
	}

	[[nodiscard]] constexpr exclusive_lock_holder<_Type, false> exclusive(std::source_location srcl = std::source_location::current()) noexcept
		requires(!_Release)
	{
		return exclusive_lock_holder<_Type, false>(*this, std::move(srcl));
	}

	[[nodiscard]] constexpr concurrent_lock_holder<_Type, false> concurrent(std::source_location srcl = std::source_location::current()) const noexcept
		requires(!_Release && constraints::is_shared_mutex_type<_Type>)
	{
		return concurrent_lock_holder<_Type, false>(const_cast<lock_owner_base<_Type, _Release>&>(*this), std::move(srcl));
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
lock_owner_base(T&&) -> lock_owner_base<T, true>;

template <class T>
lock_owner_base(T&&) -> lock_owner_base<T, false>;

template <class _Type = std::shared_mutex>
using lock_owner_release = lock_owner_base<_Type, true>;
template <class _Type = std::shared_mutex>
using lock_owner_debug = lock_owner_base<_Type, false>;

#if defined(NDEBUG) || defined(CONCURRENT_DBG_TOOLS)
template <class _Type = std::shared_mutex>
using result_lock_owner = lock_owner_release<_Type>;
#else
template <class _Type = std::shared_mutex>
using lock_owner = lock_owner_debug<_Type>;
#endif // _DEBUG

} // namespace janecekvit::synchronization
