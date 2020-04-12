/*
Copyright (c) 2019 Vit janecek <mailto:janecekvit@outlook.com>.
All rights reserved.

Concurrent.h
Purpose:	header file contains set of thread-safe concurrent containers,
			also methods that implemented over basic stl containers and
			thread-safe methods for every possiblle concurrent object


@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.05 17/03/2019
*/

//#define LOCK_PARAMS __FILE__, __func__, __LINE__

#pragma once
#include <shared_mutex>

#include <set>
#include <map>
#include <list>
#include <array>
#include <queue>
#include <stack>
#include <cerrno>
#include <vector>
#include <exception>
#include <system_error>
#include <unordered_set>
#include <unordered_map>

#include "Framework/Extensions/Constraints.h"

///Namespace owns set of thread-safe concurrent containers and methods that implemented over basic stl containers
/// and thread-safe methods for every possiblle concurrent object
namespace Concurrent
{

template <class TObject>
class ResourceOwner;

template <class TObject>
class ResourceKeeper;

/// <summary>
/// Class implements wrapper for exclusive use of input resource.
/// Input resource is locked for exclusive use, can be modified by one accessor.
/// </summary>
template <class TObject>
class ExclusiveResourceHolder
{
public:
	constexpr ExclusiveResourceHolder(const std::shared_ptr<ResourceKeeper<TObject>>& pKeeper) noexcept
		: m_pKeeper(pKeeper)
		, m_oExclusiveLock(std::unique_lock<std::shared_mutex>(*pKeeper->GetMutex()))
	{
	}

	constexpr ExclusiveResourceHolder(const ExclusiveResourceHolder&& oOther) noexcept
		: m_pKeeper(std::move(oOther.m_pKeeper))
		, m_oExclusiveLock(std::move(oOther.m_oExclusiveLock))
	{
	}

	virtual ~ExclusiveResourceHolder() = default;

	[[nodiscard]]
	constexpr operator TObject& () const
	{
		_CheckOwnership();
		return *m_pKeeper->GetResource();
	}

	[[nodiscard]]
	constexpr const std::shared_ptr<TObject> operator->() const
	{
		_CheckOwnership();
		return m_pKeeper->GetResource();
	}

	[[nodiscard]]
	constexpr TObject& Get() const
	{
		_CheckOwnership();
		return *m_pKeeper->GetResource();
	}

	constexpr void Set(TObject&& oObject) const
	{
		_CheckOwnership();
		m_pKeeper->SetResource(std::forward<TObject>(oObject));
	}

	constexpr void Swap(TObject& oObject) const
	{
		_CheckOwnership();
		m_pKeeper->SwapResource(oObject);
	}

	[[nodiscard]]
	constexpr TObject Move() const
	{
		_CheckOwnership();
		return m_pKeeper->MoveResource();
	}

	[[nodiscard]]
	constexpr TObject& operator()() const
	{
		_CheckOwnership();
		return *m_pKeeper->GetResource();
	}

	template<class TQuantified = TObject, std::enable_if_t<Constraints::is_container_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) begin() const
	{
		_CheckOwnership();
		return m_pKeeper->GetResource()->begin();
	}

	template<class TQuantified = TObject, std::enable_if_t<Constraints::is_container_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) end() const
	{
		_CheckOwnership();
		return m_pKeeper->GetResource()->end();
	}

	template<class TQuantified = TObject, std::enable_if_t<Constraints::is_container_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) size() const
	{
		_CheckOwnership();
		return m_pKeeper->GetResource()->size();
	}

	template<class TKey>
	[[nodiscard]]
	constexpr auto& operator[](const TKey& oKey)
	{
		_CheckOwnership();
		return (*m_pKeeper->GetResource())[oKey];
	}

	template<class TKey>
	[[nodiscard]]
	constexpr const auto& operator[](const TKey& oKey) const
	{
		_CheckOwnership();
		return (*m_pKeeper->GetResource())[oKey];
	}

	template <class TCondition, class TPredicate>
		requires Constraints::condition_variable_pred<TCondition, std::unique_lock<std::shared_mutex>, TPredicate>
	constexpr decltype(auto) Wait(TCondition& cv, TPredicate&& pred) const
	{
		_CheckOwnership();
		return cv.wait(m_oExclusiveLock, std::move(pred));
	}

	template <class TCondition>
		requires Constraints::condition_variable<TCondition, std::unique_lock<std::shared_mutex>>
	constexpr decltype(auto) Wait(TCondition& cv) const
	{
		_CheckOwnership();
		return cv.wait(m_oExclusiveLock);
	}

	constexpr void Release() const
	{
		_CheckOwnership();
		m_oExclusiveLock.unlock();
	}

private:
	constexpr void _CheckOwnership() const
	{
		if (!m_oExclusiveLock.owns_lock())
			throw std::system_error(EAGAIN, std::system_category().default_error_condition(EAGAIN).category(), "ExclusiveResourceHolder do not owns the resource!");
	}

private:
	const std::shared_ptr<ResourceKeeper<TObject>> m_pKeeper = nullptr;
	mutable std::unique_lock<std::shared_mutex> m_oExclusiveLock;
};

/// <summary>
/// Class implements wrapper for concurrent use of input resource.
/// Input resource is locked for concurrent use only. cannot be modified, but more accessors can read input resource.
/// </summary>
template <class TObject>
class ConcurrentResourceHolder
{
public:
	constexpr ConcurrentResourceHolder(const std::shared_ptr<const ResourceKeeper<TObject>>& pKeeper) noexcept
		: m_pKeeper(pKeeper)
		, m_oConcurrentLock(std::shared_lock<std::shared_mutex>(*pKeeper->GetMutex()))
	{
	}

	constexpr ConcurrentResourceHolder(const ConcurrentResourceHolder&& oOther) noexcept
		: m_pKeeper(std::move(oOther.m_pKeeper))
		, m_oConcurrentLock(std::move(oOther.m_oConcurrentLock))
	{
	}

	virtual ~ConcurrentResourceHolder() = default;

	[[nodiscard]]
	constexpr operator const TObject& () const
	{
		_CheckOwnership();
		return *m_pKeeper->GetResource();
	}

	[[nodiscard]]
	constexpr const std::shared_ptr<const TObject> operator->() const
	{
		_CheckOwnership();
		return m_pKeeper->GetResource();
	}

	[[nodiscard]]
	constexpr const TObject& Get() const
	{
		_CheckOwnership();
		return *m_pKeeper->GetResource();
	}

	[[nodiscard]]
	constexpr const TObject& operator()() const
	{
		_CheckOwnership();
		return *m_pKeeper->GetResource();
	}

	template<class TQuantified = TObject, std::enable_if_t<Constraints::is_container_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) begin() const
	{
		_CheckOwnership();
		return m_pKeeper->GetResource()->begin();
	}
	
	template<class TQuantified = TObject, std::enable_if_t<Constraints::is_container_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) end() const
	{
		_CheckOwnership();
		return m_pKeeper->GetResource()->end();
	}

	template<class TQuantified = TObject, std::enable_if_t<Constraints::is_container_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) size() const
	{
		_CheckOwnership();
		return m_pKeeper->GetResource()->size();
	}

	template<class TKey>
	[[nodiscard]]
	constexpr const auto& operator[](const TKey& oKey) const
	{
		_CheckOwnership();
		return (*m_pKeeper->GetResource())[oKey];
	}

	template <class TCondition, class TPredicate> 
		requires Constraints::condition_variable_pred<TCondition, std::shared_lock<std::shared_mutex>, TPredicate>
	constexpr decltype(auto) Wait(TCondition& cv, TPredicate&& pred) const
	{
		_CheckOwnership();
		return cv.wait(m_oConcurrentLock, std::move(pred));
	}

	template <class TCondition>
		requires Constraints::condition_variable<TCondition, std::shared_lock<std::shared_mutex>>
	constexpr decltype(auto) Wait(TCondition& cv) const
	{
		_CheckOwnership();
		return cv.wait(m_oConcurrentLock);
	}

	constexpr void Release() const
	{
		_CheckOwnership();
		m_oConcurrentLock.unlock();
	}


private:
	constexpr void _CheckOwnership() const
	{
		if (!m_oConcurrentLock.owns_lock())
			throw std::system_error(EAGAIN, std::system_category().default_error_condition(EAGAIN).category(), "ConcurrentResourceHolder do not owns the resource!");
	}

private:
	const std::shared_ptr<const ResourceKeeper<TObject>> m_pKeeper = nullptr;
	mutable std::shared_lock<std::shared_mutex> m_oConcurrentLock;
};

/// <summary>
/// Class implements wrapper that keeps input resources to implement  thread-safe mechanism.
/// Internal resources are saved in shared form to keep resources usable by mutliple threads until all references will be freed.
/// Class is internal helper method for Concurrent::ResourceOwner, and can be used with this class only.
/// </summary>
template <class TObject>
class ResourceKeeper
{
public:
	constexpr ResourceKeeper() = default;

	constexpr ResourceKeeper(TObject&& oObject) noexcept
		: m_pResource(std::make_shared<TObject>(std::forward<TObject>(oObject)))
	{
	}

	virtual ~ResourceKeeper() = default;

	constexpr void SetResource(TObject&& oObject)
	{
		m_pResource = std::make_shared<TObject>(std::forward<TObject>(oObject));
	}

	constexpr void SwapResource(TObject& oObject) noexcept
	{
		std::swap(oObject, *m_pResource);
	}

	[[nodiscard]]
	constexpr TObject MoveResource() noexcept
	{
		return std::move(*m_pResource);
	}

	[[nodiscard]]
	constexpr const std::shared_ptr<std::shared_mutex> GetMutex() const noexcept
	{
		return m_pMutex;
	}
	
	[[nodiscard]]
	constexpr const std::shared_ptr<TObject> GetResource() noexcept
	{
		return m_pResource;
	}

	[[nodiscard]]
	constexpr const std::shared_ptr<const TObject> GetResource() const noexcept
	{
		return m_pResource;
	}

private:
	std::shared_ptr<TObject> m_pResource = std::make_shared<TObject>();
	const std::shared_ptr<std::shared_mutex> m_pMutex = std::make_shared<std::shared_mutex>();
};

/// <summary>
/// Class implements wrapper that gains thread-safe concurrent or exclusive access to the input resource.
/// Each access method creates unique holder object that owns access to input resource for the scope.
/// Wrapper owns input resource for whole lifetime and can be extended Only by holder objects.
/// </summary>
/// <example>
/// <code>
///  Concurrent::ResourceOwner<std::unordered_map<int, int>> oMap; //Can be Concurrent::UnorderedMap<int, int> oMap;
/// 
/// // exclusive access with lifetime of one operation
/// oMap.Exclusive()->emplace(5, 3); 
/// { // exclusive access with extended lifetime for more that only one
///		auto oScope = oMap.Exclusive();
///		oScope->emplace(6, 4);
///	} // exclusive access ends
/// 
/// // Concurrent access with lifetime of one operation
/// auto iResult = oMap.Concurrent()->at(5); 
/// 
/// { // concurrent access with extended lifetime for more that only one
///		auto oScope = oMap.Concurrent();
///		auto iResultScope = oScope->at(6);
///	} // concurrent access ends
/// </code>
/// </example>
template <class TObject>
class ResourceOwner
{
public:
	constexpr ResourceOwner() = default;

	constexpr ResourceOwner(TObject&& oContainer) noexcept
		: m_pKeeper(std::make_shared<ResourceKeeper<TObject>>(std::forward<TObject>(oContainer)))
	{
	}

	virtual ~ResourceOwner()
	{
		ExclusiveResourceHolder<TObject> oFinish(m_pKeeper);
	}

	[[nodiscard]]
	constexpr ExclusiveResourceHolder<TObject> Exclusive() noexcept
	{
		return ExclusiveResourceHolder<TObject>(m_pKeeper);
	}

	[[nodiscard]]
	constexpr const ConcurrentResourceHolder<TObject> Concurrent() const noexcept
	{
		return ConcurrentResourceHolder<TObject>(m_pKeeper);
	}

private:
	mutable std::shared_ptr<ResourceKeeper<TObject>> m_pKeeper = std::make_shared<ResourceKeeper<TObject>>();
};

/// Pre-defined conversions ///

template <class ... Args>
using List = ResourceOwner<std::list<Args...>>;

template <class ... Args>
using Queue = ResourceOwner<std::queue<Args...>>;

template <class ... Args>
using Stack = ResourceOwner<std::stack<Args...>>;

template <class ... Args>
using Array = ResourceOwner<std::array<Args...>>;

template <class ... Args>
using Vector = ResourceOwner<std::vector<Args...>>;

template <class ... Args>
using Set = ResourceOwner<std::set<Args...>>;

template <class ... Args>
using Map = ResourceOwner<std::map<Args...>>;

template <class ... Args>
using MultiSet = ResourceOwner<std::multiset<Args...>>;

template <class ... Args>
using MultiMap = ResourceOwner<std::multimap<Args...>>;

template <class ... Args>
using UnorderedSet = ResourceOwner<std::unordered_set<Args...>>;

template <class ... Args>
using UnorderedMap = ResourceOwner<std::unordered_map<Args...>>;

template <class ... Args>
using UnorderedMultiSet = ResourceOwner<std::unordered_multiset<Args...>>;

template <class ... Args>
using UnorderedMultiMap = ResourceOwner<std::unordered_multimap<Args...>>;

template <class T>
using Functor = ResourceOwner<std::function<T>>;

} // namespace Concurrent 