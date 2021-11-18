/*
MIT License
Copyright (c) 2021 Vit Janecek (mailto:janecekvit@outlook.com)

concurrent.h
Purpose:	header file contains set of thread-safe concurrent containers,
			also methods that implemented over basic stl containers and
			thread-safe methods for every possible concurrent object


@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 2.00 29/10/2021
*/

#pragma once
#include "Extensions/constraints.h"

#include <array>
#include <cerrno>
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

///Namespace owns set of thread-safe concurrent containers and methods that implemented over basic stl containers
/// and thread-safe methods for every possible concurrent object
namespace concurrent
{
template <class _Type>
class resource_owner;

template <class _Type>
class ResourceKeeper;

/// <summary>
/// Class implements wrapper for exclusive use of input resource.
/// Input resource is locked for exclusive use, can be modified by one accessors.
/// </summary>
template <class _Type>
class exclusive_resource_holder
{
public:
	constexpr exclusive_resource_holder(const std::shared_ptr<ResourceKeeper<_Type>>& keeper, std::source_location&& srcl) noexcept
		: _keeper(keeper)
		, _exclusiveLock(std::unique_lock<std::shared_mutex>(*keeper->get_mutex()))
		, _srcl(srcl)
	{
		_keeper->push_exclusive_lock_information(std::move(srcl));
	}

	constexpr exclusive_resource_holder(const exclusive_resource_holder& other) noexcept = delete;

	constexpr exclusive_resource_holder(exclusive_resource_holder&& other) noexcept
		: _keeper(std::move(other._keeper))
		, _exclusiveLock(std::move(other._exclusiveLock))
		, _srcl(std::move(other._srcl))
	{
	}

	virtual ~exclusive_resource_holder()
	{
		if (_keeper)
			_keeper->pop_exclusive_lock_information();
	}

	constexpr exclusive_resource_holder& operator=(const exclusive_resource_holder& other) noexcept = delete;

	constexpr exclusive_resource_holder& operator=(exclusive_resource_holder&& other) noexcept
	{
		_keeper		   = std::move(other._keeper);
		_exclusiveLock = std::move(other._exclusiveLock);
		_srcl		   = std::move(other._srcl);
		return *this;
	}

	[[nodiscard]] constexpr operator _Type&() const
	{
		_check_ownership();
		return *_keeper->get_resource();
	}

	[[nodiscard]] constexpr const std::shared_ptr<_Type> operator->() const
	{
		_check_ownership();
		return _keeper->get_resource();
	}

	[[nodiscard]] constexpr _Type& get() const
	{
		_check_ownership();
		return *_keeper->get_resource();
	}

	constexpr void set(_Type&& object) const
	{
		_check_ownership();
		_keeper->set_resource(std::forward<_Type>(object));
	}

	constexpr void swap(_Type& object) const
	{
		_check_ownership();
		_keeper->swap_resource(object);
	}

	[[nodiscard]] constexpr _Type move() const
	{
		_check_ownership();
		return _keeper->move_resource();
	}

	[[nodiscard]] constexpr _Type& operator()() const
	{
		_check_ownership();
		return *_keeper->get_resource();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const
	{
		_check_ownership();
		return _keeper->get_resource()->begin();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const
	{
		_check_ownership();
		return _keeper->get_resource()->end();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const
	{
		_check_ownership();
		return _keeper->get_resource()->size();
	}

	template <class _Key>
	[[nodiscard]] constexpr auto& operator[](const _Key& key)
	{
		_check_ownership();
		return (*_keeper->get_resource())[key];
	}

	template <class _Key>
	[[nodiscard]] constexpr const auto& operator[](const _Key& key) const
	{
		_check_ownership();
		return (*_keeper->get_resource())[key];
	}

	constexpr void release() const
	{
		_check_ownership();
		_exclusiveLock.unlock();
	}

	constexpr void acquire() const
	{
		if (_exclusiveLock.owns_lock())
			throw std::system_error(EAGAIN, std::system_category().default_error_condition(EAGAIN).category(), "exclusive_resource_holder already owns the resource!");

		_exclusiveLock.lock();
	}

	template <class _Condition, class _Predicate>
	//requires constraints::condition_variable_pred<_Condition, std::unique_lock<std::shared_mutex>, TPredicate>
	constexpr decltype(auto) wait(_Condition& cv, _Predicate&& pred) const
	{
		_check_ownership();
		return cv.wait(_exclusiveLock, std::move(pred));
	}

	template <class _Condition>
	//requires constraints::condition_variable<_Condition, std::unique_lock<std::shared_mutex>>
	constexpr decltype(auto) wait(_Condition& cv) const
	{
		_check_ownership();
		return cv.wait(_exclusiveLock);
	}

private:
	constexpr void _check_ownership() const
	{
		if (!_exclusiveLock.owns_lock())
			throw std::system_error(EAGAIN, std::system_category().default_error_condition(EAGAIN).category(), "exclusive_resource_holder do not owns the resource!");
	}

private:
	std::source_location _srcl;
	std::shared_ptr<ResourceKeeper<_Type>> _keeper = nullptr;
	mutable std::unique_lock<std::shared_mutex> _exclusiveLock;
};

/// <summary>
/// Class implements wrapper for concurrent use of input resource.
/// Input resource is locked for concurrent use only. cannot be modified, but more accessors can read input resource.
/// </summary>
template <class _Type>
class concurrent_resource_holder
{
public:
	constexpr concurrent_resource_holder(const std::shared_ptr<const ResourceKeeper<_Type>>& keeper, std::source_location&& srcl) noexcept
		: _keeper(keeper)
		, _concurrentLock(std::shared_lock<std::shared_mutex>(*keeper->get_mutex()))
		, _srcl(srcl)
	{
		_keeper->push_concurrent_lock_information(std::move(srcl));
	}

	constexpr concurrent_resource_holder(const concurrent_resource_holder& other) noexcept = delete;

	constexpr concurrent_resource_holder(concurrent_resource_holder&& other) noexcept
		: _keeper(std::move(other._keeper))
		, _concurrentLock(std::move(other._concurrentLock))
		, _srcl(other._srcl)
	{
	}

	virtual ~concurrent_resource_holder()
	{
		if (_keeper)
			_keeper->pop_concurrent_lock_information(std::move(_srcl));
	}

	constexpr concurrent_resource_holder& operator=(const concurrent_resource_holder& other) noexcept = delete;

	constexpr concurrent_resource_holder& operator=(concurrent_resource_holder&& other) noexcept
	{
		_keeper			= std::move(other._keeper);
		_concurrentLock = std::move(other._concurrentLock);
		_srcl			= std::move(other._srcl);
		return *this;
	}

	[[nodiscard]] constexpr operator const _Type&() const
	{
		_check_ownership();
		return *_keeper->get_resource();
	}

	[[nodiscard]] constexpr const std::shared_ptr<const _Type> operator->() const
	{
		_check_ownership();
		return _keeper->get_resource();
	}

	[[nodiscard]] constexpr const _Type& get() const
	{
		_check_ownership();
		return *_keeper->get_resource();
	}

	[[nodiscard]] constexpr const _Type& operator()() const
	{
		_check_ownership();
		return *_keeper->get_resource();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const
	{
		_check_ownership();
		return _keeper->get_resource()->begin();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const
	{
		_check_ownership();
		return _keeper->get_resource()->end();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const
	{
		_check_ownership();
		return _keeper->get_resource()->size();
	}

	template <class _Key>
	[[nodiscard]] constexpr const auto& operator[](const _Key& key) const
	{
		_check_ownership();
		return (*_keeper->get_resource())[key];
	}

	constexpr void release() const
	{
		_check_ownership();
		_concurrentLock.unlock();
	}

	constexpr void acquire() const
	{
		if (_concurrentLock.owns_lock())
			throw std::system_error(EAGAIN, std::system_category().default_error_condition(EAGAIN).category(), "exclusive_resource_holder already owns the resource!");

		_concurrentLock.lock();
	}

	template <class _Condition, class _Predicate>
	//requires constraints::condition_variable_pred<_Condition, std::shared_lock<std::shared_mutex>, TPredicate>
	constexpr decltype(auto) wait(_Condition& cv, _Predicate&& pred) const
	{
		_check_ownership();
		return cv.wait(_concurrentLock, std::move(pred));
	}

	template <class _Condition>
	//requires constraints::condition_variable<_Condition, std::shared_lock<std::shared_mutex>>
	constexpr decltype(auto) wait(_Condition& cv) const
	{
		_check_ownership();
		return cv.wait(_concurrentLock);
	}

private:
	constexpr void _check_ownership() const
	{
		if (!_concurrentLock.owns_lock())
			throw std::system_error(EAGAIN, std::system_category().default_error_condition(EAGAIN).category(), "concurrent_resource_holder do not owns the resource!");
	}

private:
	std::source_location _srcl;
	std::shared_ptr<const ResourceKeeper<_Type>> _keeper = nullptr;
	mutable std::shared_lock<std::shared_mutex> _concurrentLock;
};

/// <summary>
/// Class implements wrapper that keeps input resources to implement  thread-safe mechanism.
/// Internal resources are saved in shared form to keep resources usable by mutliple threads until all references will be freed.
/// Class is internal helper method for concurrent::resource_owner, and can be used with this class only.
/// </summary>
template <class _Type>
class ResourceKeeper
{
public:
	constexpr ResourceKeeper() = default;

	constexpr ResourceKeeper(_Type&& object) noexcept
		: _resource(std::make_shared<_Type>(std::forward<_Type>(object)))
	{
	}

	virtual ~ResourceKeeper() = default;

	constexpr void set_resource(_Type&& object)
	{
		_resource = std::make_shared<_Type>(std::forward<_Type>(object));
	}

	constexpr void swap_resource(_Type& object) noexcept
	{
		std::swap(object, *_resource);
	}

	[[nodiscard]] constexpr _Type move_resource() noexcept
	{
		return std::move(*_resource);
	}

	[[nodiscard]] constexpr const std::shared_ptr<std::shared_mutex> get_mutex() const noexcept
	{
		return _mutex;
	}

	[[nodiscard]] constexpr const std::shared_ptr<_Type> get_resource() noexcept
	{
		return _resource;
	}

	[[nodiscard]] constexpr const std::shared_ptr<const _Type> get_resource() const noexcept
	{
		return _resource;
	}

	void push_exclusive_lock_information(std::source_location&& srcl)
	{
		_exlusiveLockInformation = std::move(srcl);
	}

	void pop_exclusive_lock_information() noexcept
	{
		_exlusiveLockInformation.reset();
	}

	void push_concurrent_lock_information(std::source_location&& srcl) const
	{
		_concurrentLockInformation.emplace(std::move(srcl));
	}

	void pop_concurrent_lock_information(std::source_location&& srcl) const noexcept
	{
		_concurrentLockInformation.erase(srcl);
	}

private:
	std::shared_ptr<_Type> _resource				= std::make_shared<_Type>();
	const std::shared_ptr<std::shared_mutex> _mutex = std::make_shared<std::shared_mutex>();
	std::optional<std::source_location> _exlusiveLockInformation;

	struct souce_location_comparator
	{
		constexpr bool operator()(const std::source_location& lhs, const std::source_location& rhs) const noexcept
		{
			return std::addressof(lhs) < std::addressof(rhs);
		}
	};
	mutable std::set<std::source_location, souce_location_comparator> _concurrentLockInformation;
};

/// <summary>
/// Class implements wrapper that gains thread-safe concurrent or exclusive access to the input resource.
/// Each access method creates unique holder object that owns access to input resource for the scope.
/// Wrapper owns input resource for whole lifetime and can be extended Only by holder objects.
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
template <class _Type>
class resource_owner
{
public:
	constexpr resource_owner() = default;

	constexpr resource_owner(_Type&& object) noexcept
		: _keeper(std::make_shared<ResourceKeeper<_Type>>(std::forward<_Type>(object)))
	{
	}

	virtual ~resource_owner()
	{
		exclusive_resource_holder<_Type> oFinish(_keeper, std::source_location::current());
	}

	[[nodiscard]] constexpr exclusive_resource_holder<_Type> exclusive(std::source_location srcl = std::source_location::current()) noexcept
	{
		return exclusive_resource_holder<_Type>(_keeper, std::move(srcl));
	}

	[[nodiscard]] constexpr concurrent_resource_holder<_Type> concurrent(std::source_location srcl = std::source_location::current()) const noexcept
	{
		return concurrent_resource_holder<_Type>(_keeper, std::move(srcl));
	}

private:
	mutable std::shared_ptr<ResourceKeeper<_Type>> _keeper = std::make_shared<ResourceKeeper<_Type>>();
};

/// Pre-defined conversions ///

template <class... _Args>
using list = resource_owner<std::list<_Args...>>;

template <class... _Args>
using queue = resource_owner<std::queue<_Args...>>;

template <class... _Args>
using stack = resource_owner<std::stack<_Args...>>;

template <class... _Args>
using array = resource_owner<std::array<_Args...>>;

template <class... _Args>
using vector = resource_owner<std::vector<_Args...>>;

template <class... _Args>
using set = resource_owner<std::set<_Args...>>;

template <class... _Args>
using map = resource_owner<std::map<_Args...>>;

template <class... _Args>
using multiset = resource_owner<std::multiset<_Args...>>;

template <class... _Args>
using multimap = resource_owner<std::multimap<_Args...>>;

template <class... _Args>
using unordered_set = resource_owner<std::unordered_set<_Args...>>;

template <class... _Args>
using unordered_map = resource_owner<std::unordered_map<_Args...>>;

template <class... _Args>
using unordered_multiset = resource_owner<std::unordered_multiset<_Args...>>;

template <class... _Args>
using unordered_multimap = resource_owner<std::unordered_multimap<_Args...>>;

template <class _Arg>
using functor = resource_owner<std::function<_Arg>>;

} // namespace concurrent