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

#pragma once
#include <shared_mutex>

#include <set>
#include <map>
#include <list>
#include <queue>
#include <stack>
#include <vector>
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
		ExclusiveResourceHolder(_In_ const std::shared_ptr<ResourceKeeper<TObject>> &pKeeper)
			: m_pKeeper(pKeeper)
			, m_oExclusiveLock(std::unique_lock<std::shared_mutex>(*pKeeper->GetMutex()))
		{
		}
	
		ExclusiveResourceHolder(_In_ ExclusiveResourceHolder&& oOther)
			: m_pKeeper(std::move(oOther.m_pKeeper))
			, m_oExclusiveLock(std::move(oOther.m_oExclusiveLock))
		{
		}

		virtual ~ExclusiveResourceHolder()
		{
		}

		const std::shared_ptr<TObject> operator->() const 
		{ 
			return m_pKeeper->GetResource();
		}
		TObject& Get() const
		{
			return *m_pKeeper->GetResource();
		}
		void Set(TObject && oObject) const 
		{
			m_pKeeper->SetResource(std::forward<TObject>(oObject));
		}
		const TObject& operator()() const
		{
			return *m_pKeeper->GetResource();
		}

	private:
		const std::shared_ptr<ResourceKeeper<TObject>> m_pKeeper = nullptr;
		const std::unique_lock<std::shared_mutex> m_oExclusiveLock;
	};

	/// <summary>
	/// Class implements wrapper for concurrent use of input resource.
	/// Input resource is locked for concurrent use only. cannot be modified, but more accessors can read input resource.
	/// </summary>
	template <class TObject>
	class ConcurrentResourceHolder
	{
	public:
		ConcurrentResourceHolder(_In_ const std::shared_ptr <const ResourceKeeper<TObject>> &pKeeper)
			: m_pKeeper(pKeeper)
			, m_oConcurrentLock(std::shared_lock<std::shared_mutex>(*pKeeper->GetMutex()))
		{
		}
		
		ConcurrentResourceHolder(_In_ ConcurrentResourceHolder&& oOther)
			: m_pKeeper(std::move(oOther.m_pKeeper))
			, m_oConcurrentLock(std::move(oOther.m_oConcurrentLock))
		{
		}

		virtual ~ConcurrentResourceHolder()
		{
		}

		const std::shared_ptr<const TObject> operator->() const
		{
			return m_pKeeper->GetResource();
		}
		const TObject& Get() const
		{
			return *m_pKeeper->GetResource();
		}
		const TObject& operator()() const
		{
			return *m_pKeeper->GetResource();
		}

	private:
		const std::shared_ptr<const ResourceKeeper<TObject>> m_pKeeper = nullptr;
		const std::shared_lock<std::shared_mutex> m_oConcurrentLock;
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
		ResourceKeeper()
		{
		}
		ResourceKeeper(TObject&& oObject)
			: m_pResource(std::make_shared<TObject>(std::forward<TObject>(oObject)))
		{
		}

		virtual ~ResourceKeeper()
		{
		}

		void SetResource(TObject &&oObject) { m_pResource = std::make_shared<TObject>(std::forward<TObject>(oObject)); }

		const std::shared_ptr<std::shared_mutex> GetMutex() const { return m_pMutex; }
		const std::shared_ptr<TObject> GetResource() { return m_pResource; }
		const std::shared_ptr<const TObject> GetResource() const { return m_pResource; }


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
		ResourceOwner()
		{
		}
		ResourceOwner(TObject&& oContainer)
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