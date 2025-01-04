/*
MIT License
Copyright (c) 2021 Vit Janecek (mailto:janecekvit@outlook.com)

concurrent.h
Purpose:	header file contains set of thread-safe concurrent containers,
			also methods that implemented over basic stl containers and
			thread-safe methods for every possible concurrent object


@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 2.04 9/1/2022
*/

#pragma once
#include "extensions/constraints.h"
#include "synchronization/signal.h"

#include <array>
#include <cerrno>
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
#include <unordered_map>
#include <unordered_set>
#include <vector>

/// Namespace owns set of thread-safe concurrent containers and methods that implemented over basic stl containers
///  and thread-safe methods for every possible concurrent object
namespace janecekvit::synchronization::concurrent
{
template <class _Type, bool = true>
class resource_owner;

/// <summary>
/// Class implements wrapper for exclusive use of input resource.
/// Input resource is locked for exclusive use, can be modified by one accessors.
/// </summary>
template <class _Type, bool _Release>
class exclusive_resource_holder
{
public:
	constexpr exclusive_resource_holder(resource_owner<_Type, _Release>* owner) noexcept
		: _owner(owner)
		, _unique_lock(std::unique_lock<std::shared_mutex>(*owner->_get_mutex()))
	{
	}

	constexpr exclusive_resource_holder(resource_owner<_Type, _Release>* owner, std::source_location&& srcl) noexcept
		: _owner(owner)
		, _unique_lock(std::unique_lock<std::shared_mutex>(*owner->_get_mutex()))
	{
		_owner->_push_exclusive_lock_details(std::move(srcl));
	}

	constexpr exclusive_resource_holder(const exclusive_resource_holder& other) noexcept = delete;

	constexpr exclusive_resource_holder(exclusive_resource_holder&& other) noexcept
		: _owner(std::move(other._owner))
		, _unique_lock(std::move(other._unique_lock))
	{
	}

	virtual ~exclusive_resource_holder()
	{
		if constexpr (!_Release)
		{
			if (_owner)
				_owner->_pop_exclusive_lock_details();
		}
	}

	constexpr exclusive_resource_holder& operator=(const exclusive_resource_holder& other) noexcept = delete;

	constexpr exclusive_resource_holder& operator=(exclusive_resource_holder&& other) noexcept
	{
		_owner = std::move(other._owner);
		_unique_lock = std::move(other._unique_lock);
		return *this;
	}

	[[nodiscard]] constexpr operator _Type&() const
	{
		_check_ownership();
		return *_owner->_get_resource();
	}

	[[nodiscard]] constexpr const std::shared_ptr<_Type> operator->() const
	{
		_check_ownership();
		return _owner->_get_resource();
	}

	[[nodiscard]] constexpr _Type& get() const
	{
		_check_ownership();
		return *_owner->_get_resource();
	}

	template <class _FwdType>
		requires std::is_constructible_v<_Type, _FwdType> || std::is_same_v<_Type, _FwdType>
	constexpr void set(_FwdType&& object) const
	{
		_check_ownership();
		_owner->_set_resource(std::forward<_FwdType>(object));
	}

	constexpr void swap(_Type& object) const noexcept
	{
		_check_ownership();
		_owner->_swap_resource(object);
	}

	[[nodiscard]] constexpr _Type move() const
	{
		_check_ownership();
		return _owner->_move_resource();
	}

	[[nodiscard]] constexpr _Type& operator()() const
	{
		_check_ownership();
		return *_owner->_get_resource();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const
	{
		_check_ownership();
		return _owner->_get_resource()->begin();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const
	{
		_check_ownership();
		return _owner->_get_resource()->end();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const
	{
		_check_ownership();
		return _owner->_get_resource()->size();
	}

	template <class _Key>
	[[nodiscard]] constexpr auto& operator[](const _Key& key)
	{
		_check_ownership();
		return (*_owner->_get_resource())[key];
	}

	template <class _Key>
	[[nodiscard]] constexpr const auto& operator[](const _Key& key) const
	{
		_check_ownership();
		return (*_owner->_get_resource())[key];
	}

	constexpr void unlock()
	{
		_check_ownership();
		_unique_lock.unlock();
		if constexpr (!_Release)
		{
			if (_owner)
				_owner->_pop_exclusive_lock_details();
		}
	}

	template <bool _CurrentFlag = _Release, std::enable_if_t<_CurrentFlag, int> = 0>
	constexpr void lock()
	{
		if (_unique_lock.owns_lock())
			throw std::system_error(EAGAIN, std::system_category().default_error_condition(EAGAIN).category(), "exclusive_resource_holder already owns the resource!");

		_unique_lock.lock();
	}

	template <bool _CurrentFlag = _Release, std::enable_if_t<!_CurrentFlag, int> = 0>
	constexpr void lock(std::source_location srcl = std::source_location::current())
	{
		if (_unique_lock.owns_lock())
			throw std::system_error(EAGAIN, std::system_category().default_error_condition(EAGAIN).category(), "exclusive_resource_holder already owns the resource!");

		_unique_lock.lock();
		_owner->_push_exclusive_lock_details(std::move(srcl));
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

private:
	constexpr void _check_ownership() const
	{
		if (!_unique_lock.owns_lock())
			throw std::system_error(EAGAIN, std::system_category().default_error_condition(EAGAIN).category(), "exclusive_resource_holder do not owns the resource!");
	}

private:
	resource_owner<_Type, _Release>* _owner = nullptr;
	mutable std::unique_lock<std::shared_mutex> _unique_lock;
};

/// <summary>
/// Class implements wrapper for concurrent use of input resource.
/// Input resource is locked for concurrent use only. cannot be modified, but more accessors can read input resource.
/// </summary>
template <class _Type, bool _Release>
class concurrent_resource_holder
{
public:
	constexpr concurrent_resource_holder(const resource_owner<_Type, _Release>* owner) noexcept
		: _owner(owner)
		, _shared_lock(std::shared_lock<std::shared_mutex>(*owner->_get_mutex()))
	{
	}

	constexpr concurrent_resource_holder(const resource_owner<_Type, _Release>* owner, std::source_location&& srcl) noexcept
		: _owner(owner)
		, _shared_lock(std::shared_lock<std::shared_mutex>(*owner->_get_mutex()))
	{
		_owner->_push_concurrent_lock_details(this, std::move(srcl));
	}

	constexpr concurrent_resource_holder(const concurrent_resource_holder& other) noexcept = delete;

	constexpr concurrent_resource_holder(concurrent_resource_holder&& other) noexcept
		: _owner(std::move(other._owner))
		, _shared_lock(std::move(other._shared_lock))
	{
		if constexpr (!_Release)
		{
			if (_owner && _shared_lock.owns_lock())
				_owner->_move_concurrent_lock_details(&other, this);
		}
	}

	virtual ~concurrent_resource_holder()
	{
		if constexpr (!_Release)
		{
			if (_owner)
				_owner->_pop_concurrent_lock_details(this);
		}
	}

	constexpr concurrent_resource_holder& operator=(const concurrent_resource_holder& other) noexcept = delete;

	constexpr concurrent_resource_holder& operator=(concurrent_resource_holder&& other) noexcept
	{
		_owner = std::move(other._owner);
		_shared_lock = std::move(other._shared_lock);

		if constexpr (!_Release)
		{
			if (_owner && _shared_lock.owns_lock())
				_owner->_move_concurrent_lock_details(&other, this);
		}

		return *this;
	}

	[[nodiscard]] constexpr operator const _Type&() const
	{
		_check_ownership();
		return *_owner->_get_resource();
	}

	[[nodiscard]] constexpr const std::shared_ptr<const _Type> operator->() const
	{
		_check_ownership();
		return _owner->_get_resource();
	}

	[[nodiscard]] constexpr const _Type& get() const
	{
		_check_ownership();
		return *_owner->_get_resource();
	}

	[[nodiscard]] constexpr const _Type& operator()() const
	{
		_check_ownership();
		return *_owner->_get_resource();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const
	{
		_check_ownership();
		return _owner->_get_resource()->begin();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const
	{
		_check_ownership();
		return _owner->_get_resource()->end();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const
	{
		_check_ownership();
		return _owner->_get_resource()->size();
	}

	template <class _Key>
	[[nodiscard]] constexpr const auto& operator[](const _Key& key) const
	{
		_check_ownership();
		return (*_owner->_get_resource())[key];
	}

	constexpr void unlock()
	{
		_check_ownership();
		_shared_lock.unlock();
		if constexpr (!_Release)
		{
			if (_owner)
				_owner->_pop_concurrent_lock_details(this);
		}
	}

	template <bool _CurrentFlag = _Release, std::enable_if_t<_CurrentFlag, int> = 0>
	constexpr void lock()
	{
		if (_shared_lock.owns_lock())
			throw std::system_error(EAGAIN, std::system_category().default_error_condition(EAGAIN).category(), "exclusive_resource_holder already owns the resource!");

		_shared_lock.lock();
	}

	template <bool _CurrentFlag = _Release, std::enable_if_t<!_CurrentFlag, int> = 0>
	constexpr void lock(std::source_location srcl = std::source_location::current())
	{
		if (_shared_lock.owns_lock())
			throw std::system_error(EAGAIN, std::system_category().default_error_condition(EAGAIN).category(), "exclusive_resource_holder already owns the resource!");

		_shared_lock.lock();
		_owner->_push_concurrent_lock_details(this, std::move(srcl));
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

private:
	constexpr void _check_ownership() const
	{
		if (!_shared_lock.owns_lock())
			throw std::system_error(EAGAIN, std::system_category().default_error_condition(EAGAIN).category(), "concurrent_resource_holder do not owns the resource!");
	}

private:
	const resource_owner<_Type, _Release>* _owner = nullptr;
	mutable std::shared_lock<std::shared_mutex> _shared_lock;
};

class release_resource_owner_lock_details
{
};

template <class _Type>
class resource_owner_lock_details
{
	friend exclusive_resource_holder<_Type, false>;
	friend concurrent_resource_holder<_Type, false>;

public:
	using exclusive_lock_details = typename std::optional<std::source_location>;
	using concurrent_lock_details = typename std::unordered_map<concurrent_resource_holder<_Type, false>*, std::source_location>;
	using mutex_lock_details = typename std::shared_ptr<std::mutex>;

public:
	virtual ~resource_owner_lock_details() = default;

	[[nodiscard]] constexpr exclusive_lock_details get_exclusive_lock_details() const noexcept
	{
		return _exclusive_lock_details;
	}

	[[nodiscard]] constexpr concurrent_lock_details get_concurrent_lock_details() const noexcept
	{
		return _concurrent_lock_details;
	}

private:
	void _push_exclusive_lock_details(std::source_location&& srcl)
	{
		_exclusive_lock_details = std::move(srcl);
	}

	void _pop_exclusive_lock_details() noexcept
	{
		_exclusive_lock_details.reset();
	}

	void _push_concurrent_lock_details(concurrent_resource_holder<_Type, false>* wrapper, std::source_location&& srcl) const
	{
		std::unique_lock lck(*_mutex_lock_details);
		_concurrent_lock_details.emplace(wrapper, std::move(srcl));
	}

	void _pop_concurrent_lock_details(concurrent_resource_holder<_Type, false>* wrapper) const noexcept
	{
		std::unique_lock lck(*_mutex_lock_details);
		_concurrent_lock_details.erase(wrapper);
	}

	void _move_concurrent_lock_details(concurrent_resource_holder<_Type, false>* old, concurrent_resource_holder<_Type, false>* newone) const
	{
		std::unique_lock lck(*_mutex_lock_details);
		auto&& node = _concurrent_lock_details.extract(old);
		if (node.empty())
			return;

		_concurrent_lock_details.emplace(newone, std::move(node.mapped()));
	}

protected:
	mutable mutex_lock_details _mutex_lock_details = std::make_shared<std::mutex>();
	mutable exclusive_lock_details _exclusive_lock_details;
	mutable concurrent_lock_details _concurrent_lock_details;
};

/// <summary>
/// Class implements wrapper that gains thread-safe concurrent or exclusive access to the input resource.
/// Each access method creates unique holder object that owns access to input resource for the scope.
/// Wrapper owns input resource for whole lifetime.
/// </summary>
/// <example>
/// <code>
///  concurrent::resource_owner<std::unordered_map<int, int>> oMap; //Can be concurrent::unordered_map<int, int> oMap;
///
/// // exclusive access with lifetime of one operation
/// oMap.exclusive()->emplace(5, 3);
/// { // exclusive access with extended lifetime for more that only one
///		auto oScope = oMap.exclusive();
///		oScope->emplace(6, 4);
///	} // exclusive access ends
///
/// // concurrent access with lifetime of one operation
/// auto iResult = oMap.concurrent()->at(5);
///
/// { // concurrent access with extended lifetime for more that only one
///		auto oScope = oMap.concurrent();
///		auto iResultScope = oScope->at(6);
///	} // concurrent access ends
/// </code>
/// </example>

template <class _Type, bool _Release>
class resource_owner : public std::conditional_t<!_Release, resource_owner_lock_details<_Type>, release_resource_owner_lock_details>
{
	friend exclusive_resource_holder<_Type, _Release>;
	friend concurrent_resource_holder<_Type, _Release>;

public:
	using exclusive_holder_type = exclusive_resource_holder<_Type, _Release>;
	using concurrent_holder_type = concurrent_resource_holder<_Type, _Release>;

public:
	constexpr resource_owner() = default;

	constexpr resource_owner(_Type&& object) noexcept
		: _resource(std::make_shared<_Type>(std::forward<_Type>(object)))
	{
	}

	~resource_owner()
	{
		if constexpr (!_Release)
		{
			// ensure that no operations are running on resource while destruction is in the process
			//->prevent this undefined behavior create deadlock in debugging mode
			exclusive_resource_holder<_Type, _Release> cleanup(this, std::source_location::current());
		}
	}

	[[nodiscard]] constexpr exclusive_resource_holder<_Type, true> exclusive() noexcept
		requires(_Release)
	{
		return exclusive_resource_holder<_Type, true>(this);
	}

	[[nodiscard]] constexpr concurrent_resource_holder<_Type, true> concurrent() const noexcept
		requires(_Release)
	{
		return concurrent_resource_holder<_Type, true>(this);
	}

	[[nodiscard]] constexpr exclusive_resource_holder<_Type, false> exclusive(std::source_location srcl = std::source_location::current()) noexcept
		requires(!_Release)
	{
		return exclusive_resource_holder<_Type, false>(this, std::move(srcl));
	}

	[[nodiscard]] constexpr concurrent_resource_holder<_Type, false> concurrent(std::source_location srcl = std::source_location::current()) const noexcept
		requires(!_Release)
	{
		return concurrent_resource_holder<_Type, false>(this, std::move(srcl));
	}

private:
	;

	constexpr void _set_resource(_Type&& object)
	{
		_resource = std::make_shared<_Type>(std::forward<_Type>(object));
	}

	constexpr void _swap_resource(_Type& object) noexcept
	{
		std::swap(object, *_resource);
	}

	[[nodiscard]] constexpr _Type _move_resource() noexcept
	{
		return std::move(*_resource);
	}

	[[nodiscard]] const std::shared_ptr<std::shared_mutex> _get_mutex() const noexcept
	{
		return _mutex;
	}

	[[nodiscard]] constexpr const std::shared_ptr<_Type> _get_resource() noexcept
	{
		return _resource;
	}

	[[nodiscard]] constexpr const std::shared_ptr<const _Type> _get_resource() const noexcept
	{
		return _resource;
	}

	std::shared_ptr<_Type> _resource = std::make_shared<_Type>();
	std::shared_ptr<std::shared_mutex> _mutex = std::make_shared<std::shared_mutex>();
};

template <class _Type>
using resource_owner_release = resource_owner<_Type, true>;
template <class _Type>
using resource_owner_debug = resource_owner<_Type, false>;

#if defined(NDEBUG) || defined(CONCURRENT_DBG_TOOLS)
template <class _Type>
using result_resource_owner = resource_owner_release<_Type>;
#else
template <class _Type>
using result_resource_owner = resource_owner_debug<_Type>;
#endif // _DEBUG

/// Pre-defined conversions ///
template <class... _Args>
using list = result_resource_owner<std::list<_Args...>>;

template <class... _Args>
using deque = result_resource_owner<std::deque<_Args...>>;

template <class... _Args>
using queue = result_resource_owner<std::queue<_Args...>>;

template <class... _Args>
using stack = result_resource_owner<std::stack<_Args...>>;

template <class... _Args>
using array = result_resource_owner<std::array<_Args...>>;

template <class... _Args>
using vector = result_resource_owner<std::vector<_Args...>>;

template <class... _Args>
using set = result_resource_owner<std::set<_Args...>>;

template <class... _Args>
using map = result_resource_owner<std::map<_Args...>>;

template <class... _Args>
using multiset = result_resource_owner<std::multiset<_Args...>>;

template <class... _Args>
using multimap = result_resource_owner<std::multimap<_Args...>>;

template <class... _Args>
using unordered_set = result_resource_owner<std::unordered_set<_Args...>>;

template <class... _Args>
using unordered_map = result_resource_owner<std::unordered_map<_Args...>>;

template <class... _Args>
using unordered_multiset = result_resource_owner<std::unordered_multiset<_Args...>>;

template <class... _Args>
using unordered_multimap = result_resource_owner<std::unordered_multimap<_Args...>>;

template <class _Arg>
using functor = resource_owner<std::function<_Arg>>;

} // namespace janecekvit::synchronization::concurrent
