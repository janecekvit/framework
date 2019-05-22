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

Extensions.h
Purpose:	header file contains set of extended methods implemented over stl containers

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.01 17/03/2019
*/

#pragma once
#include <list>
#include <memory>
#include <functional>
#include <type_traits>

///Namespace owns set of extended methods implemented over stl containers
namespace Extensions
{
	/// <summary>
	/// ContainerFindCallback() 
	/// Template method creates wrapper over each container that implements find method()
	/// Method used input key to search value in container and calls callback with value as the parameter.
	/// Template method deduce return type from callback's return type. 
	/// </summary>
	/// <param name="oContainer">The input container defined by begin() and end() iterators.</param>
	/// <param name="oKey">The input key to search value in container.</param>
	/// <param name="oCallback">The lambda/functor callback what is called when value was found in container.</param>
	/// <returns>Deduced return type from callback's retrun type</returns>
	/// <example>
	/// <code>
	///	std::unordered_map<int, int> mapInts;
	///	mapInts.emplace(5, 10);
	///	auto iResult = Extensions::ContainerFind(mapInts, 5, [](_In_ int &oResult)
	///	{
	///		oResult += 1;
	///		return oResult;
	///	});
	/// </code>
	/// </example>
	template <template <class ...> class Container, class Key, class ... Args, class Functor>
	auto ContainerFind(
		_In_ Container<Args...>& oContainer,
		_In_ const Key& oKey,
		_In_ Functor&& oCallback)
		-> decltype(std::begin(oContainer), std::end(oContainer), oContainer.find(oKey), oCallback(oContainer.find(oKey)->second))
	{
		if constexpr (std::is_same_v<decltype(oCallback(oContainer.find(oKey)->second)), void>)
		{	// void return type
			auto&& it = oContainer.find(oKey);
			if (it != oContainer.end())
				oCallback(it->second);
		}
		else
		{	// return type is non-void
			decltype(oCallback(oContainer.find(oKey)->second)) oResult = {};
			auto&& it = oContainer.find(oKey);
			if (it != oContainer.end())
				oResult = oCallback(it->second);
			return oResult;
		}
	}

	template <template <class ...> class Container, class Key, class ... Args, class Functor>
	auto ContainerFind(
		_In_ const Container<Args...>& oContainer,
		_In_ const Key& oKey,
		_In_ Functor&& oCallback)
		-> decltype(std::begin(oContainer), std::end(oContainer), oContainer.find(oKey), oCallback(oContainer.find(oKey)->second)) const
	{
		return ContainerFind(const_cast<Container<Args...>&> (oContainer), oKey, oCallback);
	}

	/// <summary>
	/// ForEach() method internally calls std::ForEach method with lambda predicator over input collection
	/// </summary>
	/// <param name="oContainer">The input container defined by begin() and end() iterators.</param>
	/// <param name="oCallback">The lambda/functor callback called for each value in container.</param>
	/// <returns>void()</returns>
	template <template <class ...> class Container, class ... Args, class Functor>
	auto ForEach(
		_In_ Container<Args...>& oContainer,
		_In_ Functor&& oCallback)
		-> decltype(std::begin(oContainer), std::end(oContainer), void())
	{
		std::for_each(oContainer.begin(), oContainer.end(), oCallback);
	}
	template <template <class ...> class Container, class ... Args, class Functor>
	auto ForEach(
		_In_ const Container<Args...>& oContainer,
		_In_ Functor&& oCallback)
		-> decltype(std::begin(oContainer), std::end(oContainer), void()) const
	{
		return ForEach(const_cast<Container<Args...>&> (oContainer), oCallback);
	}

	/// <summary>
	/// AnyOf() method internally calls std::any_of method with  lambda predicator over input collection
	/// <param name="oContainer">The input container defined by begin() and end() iterators.</param>
	/// <param name="oCallback">The lambda/functor callback called for each value in container.</param>
	/// <returns>return true, when callback predicate found result for any item in container</returns>
	/// </summary>
	template <template <class ...> class Container, class ... Args, class Functor>
	auto AnyOf(
		_In_ Container<Args...>& oContainer,
		_In_ Functor&& oCallback)
		-> decltype(std::begin(oContainer), std::end(oContainer), oCallback(*std::begin(oContainer)), bool())
	{
		return std::any_of(oContainer.begin(), oContainer.end(), oCallback);
	}
	template <template <class ...> class Container, class ... Args, class Functor>
	auto AnyOf(
		_In_ const Container<Args...> & oContainer,
		_In_ Functor && oCallback)
		-> decltype(std::begin(oContainer), std::end(oContainer), oCallback(*std::begin(oContainer)), bool()) const
	{
		return AnyOf(const_cast<Container<Args...>&> (oContainer), oCallback);
	}

	/// <summary>
	/// Method implements RAII memory wrapper recasting from TBase to TDervied
	/// Method convert current instance of RAII memory wrapper to the new one
	/// Method is atomically, if recast cannot be done, input pointer is still valid
	/// </summary>
	/// <returns>On success, returns recasted memory-safe pointer, else do not nothing</returns>
	template <class TBase, class TDerived>
	std::unique_ptr<TDerived> Recast(
		_In_ std::unique_ptr<TBase>& pItem)
	{
		auto* pTemp = dynamic_cast<TDerived*>(pItem.get());
		if (pTemp)
			return nullptr;

		pItem.release();
		return std::unique_ptr<TDerived>(pTemp);
	}


	/// <summary>
	/// Method implements RAII memory wrapper recasting from TDervied to TBase
	/// Method convert current instance of RAII memory wrapper to the new one
	/// Method is atomically, if recast cannot be done, input pointer is still valid
	/// </summary>
	/// <returns>On success, returns recasted memory-safe pointer, else do not nothing</returns>
	template <class TBase, class TDerived>
	std::unique_ptr<TBase> Recast(
		_In_ std::unique_ptr<TDerived>& pItem)
	{
		auto* pTemp = dynamic_cast<TBase*>(pItem.get());
		if (pTemp)
			return nullptr;

		pItem.release();
		return std::unique_ptr<TBase>(pTemp);
	}


	/// <summary>
	/// Parameter pack class can forward input variadic argument list to the any object for future processing
	/// Parameter pack implement lazy evaluation idiom to enable processing input arguments as late as possible
	/// </summary>
	class ParameterPack
	{
	public:
		virtual ~ParameterPack() = default;

		template <class ... Args>
		ParameterPack(
			_In_ const Args& ... oArgs)
		{
			_SerializeParameters(oArgs...);
		}


	public:
		template <class ... Args>
		void GetPack(
			_Inout_ Args& ... oArgs)
		{

			if (m_listArgs.size() != sizeof...(Args))
				return;

			auto listArgs = m_listArgs;
			_DeserializeParameters(std::move(listArgs), oArgs...);
		}

	protected:
		template <class T>
		void _SerializeParameters(
			_In_ const T& oFirst)
		{
			m_listArgs.emplace_back((void*) oFirst);
		}

		template <class T, class... Args>
		void _SerializeParameters(
			_In_ const T& oFirst,
			_In_ const Args& ... oRest)
		{
			_SerializeParameters(oFirst);
			_SerializeParameters(oRest...);
		}

		template <class T>
		void _DeserializeParameters(
			_In_ std::list<void*>&& listArgs,
			_Inout_ T& oFirst)
		{
			oFirst = (T) listArgs.front();
			listArgs.pop_front();
		}

		template <class T, class... Args>
		void _DeserializeParameters(
			_In_ std::list<void*>&& listArgs,
			_Inout_ T& oFirst,
			_Inout_ Args& ... oRest)
		{
			_DeserializeParameters(std::move(listArgs), oFirst);
			_DeserializeParameters(std::move(listArgs), oRest...);
		}

	protected:
		std::list<void*> m_listArgs;
	};


} //namespace Extensions
