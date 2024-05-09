/*
MIT License
Copyright (c) 2021 Vit Janecek (mailto:janecekvit@outlook.com)

concurrent.h
Purpose:	header file contains set of atomic thread-safe concurrent containers,
			also methods that implemented over basic stl containers and
			thread-safe methods for every possible concurrent object


@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 0.81 31/12/2021
TODO: 1. std::aligned_storage (aligned_storage_t) for writer deep copy 
TODO: 2. Make atomic_concurrent based on two classes (exclusive writer, more writers with std::merge class)
*/

#pragma once
#include "extensions/constraints.h"

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

namespace janecekvit::synchronization::atomic_concurrent
{
template <class _Type>
class resource_owner;

/// <summary>
/// Class implements wrapper for exclusive use of input resource.
/// Input resource is locked for exclusive use, can be modified by one accessors.
/// </summary>
template <class _Type>
class resource_writer
{
public:
	constexpr resource_writer(resource_owner<_Type>* owner, std::source_location&& srcl) noexcept
		: _owner(owner)
		, _srcl(srcl)
	{
		const auto&& oldType = _owner->_get_value();
		_resource			 = std::make_shared<_Type>(*oldType);

		//_resource->push_exclusive_lock_details(std::move(srcl));
	}

	constexpr resource_writer(const resource_writer& other) noexcept = delete;

	constexpr resource_writer(resource_writer&& other) noexcept
		: _resource(std::move(other._resource))
		, _owner(other._owner)
		, _srcl(other._srcl)
	{
	}

	virtual ~resource_writer()
	{
		if (_resource && _owner)
			_owner->_update_value(std::move(_resource));
		/*if (_resource)
			_resource->pop_exclusive_lock_details();*/
	}

	constexpr resource_writer& operator=(const resource_writer& other) noexcept = delete;

	constexpr resource_writer& operator=(resource_writer&& other) noexcept
	{
		_resource = std::move(other._resource);
		_owner	  = std::move(other._owner);
		_srcl	  = std::move(other._srcl);
		return *this;
	}

	[[nodiscard]] constexpr operator _Type&() const
	{
		return *_resource;
	}

	[[nodiscard]] constexpr const std::shared_ptr<_Type> operator->() const
	{
		return _resource;
	}

	[[nodiscard]] constexpr _Type& get() const
	{
		return *_resource;
	}

	constexpr void set(_Type&& object)
	{
		_resource = std::make_shared<_Type>(std::forward<_Type>(object));
	}

	constexpr void swap(_Type& object)
	{
		std::swap(*_resource, object);
	}

	[[nodiscard]] constexpr _Type move()
	{
		return std::move(*_resource);
	}

	[[nodiscard]] constexpr _Type& operator()() const
	{
		return *_resource;
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const
	{
		return _resource->begin();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const
	{
		return _resource->end();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const
	{
		return _resource->size();
	}

	template <class _Key>
	[[nodiscard]] constexpr auto& operator[](const _Key& key)
	{
		return (*_resource)[key];
	}

	template <class _Key>
	[[nodiscard]] constexpr const auto& operator[](const _Key& key) const
	{
		return (*_resource)[key];
	}

private:
	std::shared_ptr<_Type> _resource;
	resource_owner<_Type>* _owner = nullptr;
	std::source_location _srcl;
};

/// <summary>
/// Class implements wrapper for concurrent use of input resource.
/// Input resource is locked for concurrent use only. cannot be modified, but more accessors can read input resource.
/// </summary>
template <class _Type>
class resource_reader
{
public:
	constexpr resource_reader(const resource_owner<_Type>* owner, std::source_location&& srcl) noexcept
		: _resource(owner->_get_value())
		, _owner(owner)
		, _srcl(srcl)
	{
		//_resource->push_concurrent_lock_details(std::move(srcl));
	}

	constexpr resource_reader(const resource_reader& other) noexcept = delete;

	constexpr resource_reader(resource_reader&& other) noexcept
		: _resource(std::move(other._resource))
		, _owner(std::move(other._owner))
		, _srcl(other._srcl)
	{
	}

	virtual ~resource_reader()
	{
		/*if (_resource)
			_resource->pop_concurrent_lock_details(std::move(_srcl));*/
	}

	constexpr resource_reader& operator=(const resource_reader& other) noexcept = delete;

	constexpr resource_reader& operator=(resource_reader&& other) noexcept
	{
		_resource = std::move(other._resource);
		_owner	  = std::move(other._owner);
		_srcl	  = std::move(other._srcl);
		return *this;
	}

	[[nodiscard]] constexpr operator const _Type&() const
	{
		return *_resource;
	}

	[[nodiscard]] constexpr const std::shared_ptr<const _Type> operator->() const
	{
		return _resource;
	}

	[[nodiscard]] constexpr const _Type& get() const
	{
		return *_resource;
	}

	[[nodiscard]] constexpr const _Type& operator()() const
	{
		return *_resource;
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const
	{
		return _resource->begin();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const
	{
		return _resource->end();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const
	{
		return _resource->size();
	}

	template <class _Key>
	[[nodiscard]] constexpr const auto& operator[](const _Key& key) const
	{
		return (*_resource)[key];
	}

private:
	std::shared_ptr<const _Type> _resource;
	const resource_owner<_Type>* _owner = nullptr;
	std::source_location _srcl;
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
	friend class resource_writer<_Type>;
	friend class resource_reader<_Type>;

public:
	constexpr resource_owner() = default;

	constexpr resource_owner(_Type&& object) noexcept
		: _resource(std::make_shared<_Type>(std::forward<_Type>(object)))
	{
	}

	constexpr resource_owner<_Type>& operator=(resource_owner<_Type>&& other)
	{
		_resource = std::move(other._resource.load());
		return *this;
	}

	virtual ~resource_owner()
	{
		_stopSource.request_stop();
	}

	[[nodiscard]] constexpr resource_writer<_Type> writer(std::source_location srcl = std::source_location::current())
	{
		std::unique_lock lock(_writerMutex);
		_writerSignal.wait(lock, _stopSource.get_token(), [&]()
			{
				return !_writerAcquired || _stopSource.stop_requested();
			});

		if (_stopSource.stop_requested())
			throw std::exception("Break all waiting threads due container destruction.");

		_writerAcquired = true;
		return resource_writer<_Type>(this, std::move(srcl));
	}

	[[nodiscard]] constexpr std::optional<resource_writer<_Type>> try_to_acquire_writer(std::source_location srcl = std::source_location::current()) noexcept
	{
		if (_writerAcquired)
			return {};

		std::unique_lock lock(_writerMutex);
		if (_writerAcquired)
			return {};

		_writerAcquired = true;
		return resource_writer<_Type>(this, std::move(srcl));
	}

	[[nodiscard]] constexpr resource_reader<_Type> reader(std::source_location srcl = std::source_location::current()) const noexcept
	{
		return resource_reader<_Type>(this, std::move(srcl));
	}

	constexpr bool is_writer_free() const
	{
		return !_writerAcquired;
	}

private:
	constexpr decltype(auto) _get_value() const
	{
		return _resource.load();
	}

	constexpr void _update_value(std::shared_ptr<_Type>&& value)
	{
		_resource.store(std::move(value));
		_writerAcquired = false;
		_writerSignal.notify_one();
	}

private:
	mutable std::atomic<std::shared_ptr<_Type>> _resource = std::make_shared<_Type>();
	std::atomic<bool> _writerAcquired;

	std::mutex _writerMutex;
	std::stop_source _stopSource;
	std::condition_variable_any _writerSignal;
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

} // namespace janecekvit::synchronization::atomic_concurrent