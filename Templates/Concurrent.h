/*
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
Copyright (c) 2019 Vit janecek <mailto:janecekvit@outlook.com>.

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Concurrent.h
Purpose:	header file contains set of thread-safe concurrent containers,
			also methods that implemented over basic stl containers and
			thread-safe methods for every possiblle concurrent object


@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.02 17/03/2019
*/

//USE IT
#define LOCK_PARAMS __FILE__, __func__, __LINE__

#pragma once
#include <shared_mutex>

#include <set>
#include <map>
#include <list>
#include <queue>
#include <stack>
#include <cerrno>
#include <vector>
#include <exception>
#include <system_error>
#include <unordered_set>
#include <unordered_map>

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
	ExclusiveResourceHolder(_In_ const std::shared_ptr<ResourceKeeper<TObject>>& pKeeper) noexcept
		: m_pKeeper(pKeeper)
		, m_oExclusiveLock(std::unique_lock<std::shared_mutex>(*pKeeper->GetMutex()))
	{
	}

	ExclusiveResourceHolder(_In_ const ExclusiveResourceHolder&& oOther) noexcept
		: m_pKeeper(std::move(oOther.m_pKeeper))
		, m_oExclusiveLock(std::move(oOther.m_oExclusiveLock))
	{
	}

	virtual ~ExclusiveResourceHolder() = default;

	const std::shared_ptr<TObject> operator->() const
	{
		_CheckOwnership();
		return m_pKeeper->GetResource();
	}
	TObject& Get() const
	{
		_CheckOwnership();
		return *m_pKeeper->GetResource();
	}

	void Set(_In_ TObject&& oObject) const
	{
		_CheckOwnership();
		m_pKeeper->SetResource(std::forward<TObject>(oObject));
	}

	TObject& operator()() const
	{
		_CheckOwnership();
		return *m_pKeeper->GetResource();
	}

	decltype(auto) begin() const
	{
		_CheckOwnership();
		return m_pKeeper->GetResource()->begin();
	}

	decltype(auto) end() const
	{
		_CheckOwnership();
		return m_pKeeper->GetResource()->end();
	}

	template<class TKey>
	auto& operator[](_In_ const TKey &oKey)
	{
		_CheckOwnership();
		return (*m_pKeeper->GetResource())[oKey];
	}

	template<class TKey>
	const auto& operator[](_In_ const TKey& oKey) const
	{
		_CheckOwnership();
		return (*m_pKeeper->GetResource())[oKey];
	}

	void Release() const
	{
		_CheckOwnership();
		m_oExclusiveLock.unlock();
	}

private:
	void _CheckOwnership() const
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
	ConcurrentResourceHolder(_In_ const std::shared_ptr<const ResourceKeeper<TObject>>& pKeeper) noexcept
		: m_pKeeper(pKeeper)
		, m_oConcurrentLock(std::shared_lock<std::shared_mutex>(*pKeeper->GetMutex()))
	{
	}

	ConcurrentResourceHolder(_In_ const ConcurrentResourceHolder&& oOther) noexcept
		: m_pKeeper(std::move(oOther.m_pKeeper))
		, m_oConcurrentLock(std::move(oOther.m_oConcurrentLock))
	{
	}

	virtual ~ConcurrentResourceHolder() = default;

	const std::shared_ptr<const TObject> operator->() const
	{
		_CheckOwnership();
		return m_pKeeper->GetResource();
	}
	const TObject& Get() const
	{
		_CheckOwnership();
		return *m_pKeeper->GetResource();
	}
	const TObject& operator()() const
	{
		_CheckOwnership();
		return *m_pKeeper->GetResource();
	}

	decltype(auto) begin() const
	{
		_CheckOwnership();
		return m_pKeeper->GetResource()->begin();
	}

	decltype(auto) end() const
	{
		_CheckOwnership();
		return m_pKeeper->GetResource()->end();
	}

	template<class TKey>
	const auto& operator[](_In_ const TKey& oKey) const
	{
		_CheckOwnership();
		return (*m_pKeeper->GetResource())[oKey];
	}

	void Release() const
	{
		_CheckOwnership();
		m_oConcurrentLock.unlock();
	}


private:
	void _CheckOwnership() const
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
	virtual ~ResourceKeeper() = default;
	ResourceKeeper(_In_ TObject&& oObject) noexcept
		: m_pResource(std::make_shared<TObject>(std::forward<TObject>(oObject)))
	{
	}

	ResourceKeeper() = default;

	void SetResource(_In_ TObject&& oObject)
	{
		m_pResource = std::make_shared<TObject>(std::forward<TObject>(oObject));
	}

	const std::shared_ptr<std::shared_mutex> GetMutex() const
	{
		return m_pMutex;
	}
	const std::shared_ptr<TObject> GetResource()
	{
		return m_pResource;
	}
	const std::shared_ptr<const TObject> GetResource() const
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
	ResourceOwner() = default;
	ResourceOwner(_In_ TObject&& oContainer) noexcept
		: m_pKeeper(std::make_shared<ResourceKeeper<TObject>>(std::forward<TObject>(oContainer)))
	{
	}
	virtual ~ResourceOwner()
	{
		ExclusiveResourceHolder<TObject> oFinish(m_pKeeper);
	}

	ExclusiveResourceHolder<TObject> Exclusive()
	{
		return ExclusiveResourceHolder<TObject>(m_pKeeper);
	}
	const ConcurrentResourceHolder<TObject> Concurrent() const
	{
		return ConcurrentResourceHolder<TObject>(m_pKeeper);
	}

private:
	mutable std::shared_ptr<ResourceKeeper<TObject>> m_pKeeper = std::make_shared<ResourceKeeper<TObject>>();
};

/// Pre-defined conversions ///

template <class ... Args>
using Set = ResourceOwner<std::set<Args...>>;

template <class ... Args>
using Map = ResourceOwner<std::map<Args...>>;

template <class ... Args>
using List = ResourceOwner<std::list<Args...>>;

template <class ... Args>
using Queue = ResourceOwner<std::queue<Args...>>;

template <class ... Args>
using Stack = ResourceOwner<std::stack<Args...>>;

template <class ... Args>
using Vector = ResourceOwner<std::vector<Args...>>;

template <class ... Args>
using UnorderedSet = ResourceOwner<std::unordered_set<Args...>>;

template <class ... Args>
using UnorderedMap = ResourceOwner<std::unordered_map<Args...>>;

} // namespace Concurrent 