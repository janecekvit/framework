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
#include "synchronization/lock_owner.h"
#include "synchronization/signal.h"

#include <array>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <shared_mutex>
#include <source_location>
#include <stack>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

/// Namespace owns set of thread-safe concurrent containers and methods that implemented over basic stl containers
///  and thread-safe methods for every possible concurrent object
namespace janecekvit::synchronization::concurrent
{
template <class _Type, lock_tracking_policy _Policy = lock_tracking_disabled>
class resource_owner;

/// <summary>
/// Class implements wrapper for exclusive use of input resource.
/// Input resource is locked for exclusive use, can be modified by one accessors.
/// </summary>
template <class _Type, lock_tracking_policy _Policy>
class [[nodiscard]] exclusive_resource_holder : public exclusive_lock_holder<std::shared_mutex, _Policy>
{
	using base_type = exclusive_lock_holder<std::shared_mutex, _Policy>;

public:
	constexpr exclusive_resource_holder(resource_owner<_Type, _Policy>& owner) noexcept
		: base_type(owner)
		, _owner(&owner)
	{
	}

	constexpr exclusive_resource_holder(resource_owner<_Type, _Policy>& owner, std::source_location&& srcl) noexcept
		: base_type(owner, std::move(srcl), typeid(_Type))
		, _owner(&owner)
	{
	}

	virtual ~exclusive_resource_holder() = default;

	constexpr exclusive_resource_holder(const exclusive_resource_holder& other) noexcept = delete;

	constexpr exclusive_resource_holder(exclusive_resource_holder&& other) noexcept
		: base_type(std::move(other))
		, _owner(other._owner)
	{
	}

	constexpr exclusive_resource_holder& operator=(const exclusive_resource_holder& other) noexcept = delete;

	constexpr exclusive_resource_holder& operator=(exclusive_resource_holder&& other) noexcept
	{
		base_type::operator=(std::move(other));
		_owner = std::move(other._owner);
		return *this;
	}

	[[nodiscard]] constexpr operator _Type&() const
	{
		this->_check_ownership();
		return *_owner->_get_resource();
	}

	[[nodiscard]] constexpr const std::shared_ptr<_Type> operator->() const
	{
		this->_check_ownership();
		return _owner->_get_resource();
	}

	[[nodiscard]] constexpr _Type& get() const
	{
		this->_check_ownership();
		return *_owner->_get_resource();
	}

	template <class _FwdType>
		requires std::is_constructible_v<_Type, _FwdType> || std::is_same_v<_Type, _FwdType>
	constexpr void set(_FwdType&& object) const
	{
		this->_check_ownership();
		_owner->_set_resource(std::forward<_FwdType>(object));
	}

	constexpr void swap(_Type& object) const noexcept
	{
		this->_check_ownership();
		_owner->_swap_resource(object);
	}

	[[nodiscard]] constexpr _Type move() const
	{
		this->_check_ownership();
		return _owner->_move_resource();
	}

	[[nodiscard]] constexpr _Type& operator()() const
	{
		this->_check_ownership();
		return *_owner->_get_resource();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const
	{
		this->_check_ownership();
		return _owner->_get_resource()->begin();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const
	{
		this->_check_ownership();
		return _owner->_get_resource()->end();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const
	{
		this->_check_ownership();
		return _owner->_get_resource()->size();
	}

	template <class _Key>
	[[nodiscard]] constexpr auto& operator[](const _Key& key)
	{
		this->_check_ownership();
		return (*_owner->_get_resource())[key];
	}

	template <class _Key>
	[[nodiscard]] constexpr const auto& operator[](const _Key& key) const
	{
		this->_check_ownership();
		return (*_owner->_get_resource())[key];
	}

private:
	resource_owner<_Type, _Policy>* _owner;
};

/// <summary>
/// Class implements wrapper for concurrent use of input resource.
/// Input resource is locked for concurrent use only. cannot be modified, but more accessors can read input resource.
/// </summary>
template <class _Type, lock_tracking_policy _Policy>
class [[nodiscard]] concurrent_resource_holder : public concurrent_lock_holder<std::shared_mutex, _Policy>
{
	using base_type = concurrent_lock_holder<std::shared_mutex, _Policy>;

public:
	constexpr concurrent_resource_holder(resource_owner<_Type, _Policy>& owner) noexcept
		: base_type(owner)
		, _owner(&owner)
	{
	}

	constexpr concurrent_resource_holder(resource_owner<_Type, _Policy>& owner, std::source_location&& srcl) noexcept
		: base_type(owner, std::move(srcl), typeid(_Type))
		, _owner(&owner)
	{
	}

	~concurrent_resource_holder() = default;

	constexpr concurrent_resource_holder(const concurrent_resource_holder& other) noexcept = delete;

	constexpr concurrent_resource_holder(concurrent_resource_holder&& other) noexcept
		: base_type(std::move(other))
		, _owner(other._owner)
	{
	}

	constexpr concurrent_resource_holder& operator=(const concurrent_resource_holder& other) noexcept = delete;

	constexpr concurrent_resource_holder& operator=(concurrent_resource_holder&& other) noexcept
	{
		base_type::operator=(std::move(other));
		_owner = std::move(other._owner);
		return *this;
	}

	[[nodiscard]] constexpr operator const _Type&() const
	{
		this->_check_ownership();
		return *_owner->_get_resource();
	}

	[[nodiscard]] constexpr const std::shared_ptr<const _Type> operator->() const
	{
		this->_check_ownership();
		return _owner->_get_resource();
	}

	[[nodiscard]] constexpr const _Type& get() const
	{
		this->_check_ownership();
		return *_owner->_get_resource();
	}

	[[nodiscard]] constexpr const _Type& operator()() const
	{
		this->_check_ownership();
		return *_owner->_get_resource();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const
	{
		this->_check_ownership();
		return _owner->_get_resource()->begin();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const
	{
		this->_check_ownership();
		return _owner->_get_resource()->end();
	}

	template <class _Quantified = _Type, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const
	{
		this->_check_ownership();
		return _owner->_get_resource()->size();
	}

	template <class _Key>
	[[nodiscard]] constexpr const auto& operator[](const _Key& key) const
	{
		this->_check_ownership();
		return (*_owner->_get_resource())[key];
	}

private:
	resource_owner<_Type, _Policy>* _owner;
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

template <class _Type, lock_tracking_policy _Policy>
class [[nodiscard]] resource_owner : public lock_owner_base<std::shared_mutex, _Policy>
{
	friend exclusive_resource_holder<_Type, _Policy>;
	friend concurrent_resource_holder<_Type, _Policy>;

	using base_type = lock_owner_base<std::shared_mutex, _Policy>;

public:
	using exclusive_holder_type = exclusive_resource_holder<_Type, _Policy>;
	using concurrent_holder_type = concurrent_resource_holder<_Type, _Policy>;

public:
	constexpr resource_owner() = default;
	constexpr ~resource_owner() = default;

	constexpr resource_owner(_Type&& object) noexcept
		: base_type()
		, _resource(std::make_shared<_Type>(std::forward<_Type>(object)))
	{
	}

	// For compile-time disabled tracking
	[[nodiscard]] constexpr auto exclusive() noexcept
		requires(_Policy::is_compile_time && !_Policy::should_track())
	{
		return exclusive_resource_holder<_Type, _Policy>(*this);
	}

	// For compile-time enabled tracking
	[[nodiscard]] constexpr auto exclusive(std::source_location srcl = std::source_location::current()) noexcept
		requires(_Policy::is_compile_time && _Policy::should_track())
	{
		return exclusive_resource_holder<_Type, _Policy>(*this, std::move(srcl));
	}

	// For runtime tracking
	[[nodiscard]] auto exclusive(std::source_location srcl = std::source_location::current()) noexcept
		requires(!_Policy::is_compile_time)
	{
		if (_Policy::should_track())
		{
			return exclusive_resource_holder<_Type, _Policy>(*this, std::move(srcl));
		}
		return exclusive_resource_holder<_Type, _Policy>(*this);
	}

	// For compile-time disabled tracking
	[[nodiscard]] constexpr auto concurrent() const noexcept
		requires(_Policy::is_compile_time && !_Policy::should_track())
	{
		return concurrent_resource_holder<_Type, _Policy>(const_cast<resource_owner<_Type, _Policy>&>(*this));
	}

	// For compile-time enabled tracking
	[[nodiscard]] constexpr auto concurrent(std::source_location srcl = std::source_location::current()) const noexcept
		requires(_Policy::is_compile_time && _Policy::should_track())
	{
		return concurrent_resource_holder<_Type, _Policy>(const_cast<resource_owner<_Type, _Policy>&>(*this), std::move(srcl));
	}

	// For runtime tracking
	[[nodiscard]] auto concurrent(std::source_location srcl = std::source_location::current()) const noexcept
		requires(!_Policy::is_compile_time)
	{
		if (_Policy::should_track())
		{
			return concurrent_resource_holder<_Type, _Policy>(const_cast<resource_owner<_Type, _Policy>&>(*this), std::move(srcl));
		}
		return concurrent_resource_holder<_Type, _Policy>(const_cast<resource_owner<_Type, _Policy>&>(*this));
	}

private:
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

	[[nodiscard]] constexpr const std::shared_ptr<_Type> _get_resource() noexcept
	{
		return _resource;
	}

	[[nodiscard]] constexpr const std::shared_ptr<const _Type> _get_resource() const noexcept
	{
		return _resource;
	}

	std::shared_ptr<_Type> _resource = std::make_shared<_Type>();
};

/// <summary>
/// Compile-time aliases for resource owner tracking
/// </summary>
template <class _Type>
using resource_owner_release = resource_owner<_Type, lock_tracking_disabled>;

template <class _Type>
using resource_owner_debug = resource_owner<_Type, lock_tracking_enabled>;

/// <summary>
/// Runtime alias for lock tracking
/// </summary>
template <class _Type>
using resource_owner_runtime = resource_owner<_Type, lock_tracking_runtime>;

/// <summary>
/// Build configuration alias
/// </summary>
#if defined(NDEBUG) || defined(SYNCHRONIZATION_NO_TRACKING)
template <class _Type>
using result_resource_owner = resource_owner_release<_Type>;
#elif defined(SYNCHRONIZATION_RUNTIME_TRACKING)
template <class _Type>
using result_resource_owner = resource_owner_runtime<_Type>;
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
