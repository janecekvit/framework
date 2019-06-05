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
@version 1.07 17/03/2019
*/

#pragma once
#include <list>
#include <tuple>
#include <memory>
#include <functional>
#include <type_traits>

///Namespace owns set of extended methods implemented over stl containers
namespace Extensions
{
	/// <summary>
	/// Helper structures to determine if template type <T> is shared_ptr
	/// </summary>
	template<class T> struct is_shared_ptr_helper : std::false_type {};
	template<class T> struct is_shared_ptr_helper<std::shared_ptr<T>> : std::true_type {};
	template<class T> struct is_shared_ptr : is_shared_ptr_helper<typename std::remove_cv<T>::type> {};

	/// <summary>
	/// Helper structures to determine if template type <T> is unique_ptr
	/// </summary>
	template<class T> struct is_unique_ptr_helper : std::false_type {};
	template<class T> struct is_unique_ptr_helper<std::unique_ptr<T>> : std::true_type {};
	template<class T> struct is_unique_ptr : is_unique_ptr_helper<typename std::remove_cv<T>::type> {};

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

	// Helper base class used as wrapper to hold any type described by derived Parameter<T> class
	class ParameterBase
	{
	public:
		virtual ~ParameterBase() = default;
		template <class T>
		const T& Get() const; // Method is implemented after Parameter derived class, to gain ability to operate with it
	};

	// Helper derived class used as container of any input type. 
	template <class T>
	class Parameter : public virtual ParameterBase
	{
	public:
		virtual ~Parameter() = default;
		Parameter(_In_ const T& oValue)
			: m_oValue(oValue)
		{
		}

		const T& Get() const
		{
			return m_oValue;
		}

	protected:
		T m_oValue = {};
	};

	// Core method: create dynamic_cast instead of virtual cast to get type what allocated in derived class
	template<class T>
	const T& ParameterBase::Get() const
	{
		using TRetrievedType = typename std::remove_cv<typename std::remove_reference<decltype(std::declval<Parameter<T>>().Get())>::type>::type;
		static_assert(std::is_same<T, TRetrievedType>::value, "Cannot cast templated return type <T> to the derived class \"Parameter.<T>Get()\" type!");

		return dynamic_cast<const Parameter<T>&>(*this).Get();
	}

	/// <summary>
	/// Parameter pack class can forward input variadic argument's list to the any object for future processing
	/// Parameter pack implement lazy evaluation idiom to enable processing input arguments as late as possible
	/// Packed parameters can be retrieved from pack by out parameters or by return value through std::tuple 
	/// </summary>
	/// <example>
	/// <code>
	/// 
	/// struct IInterface
	/// {
	/// 	virtual ~IInterface() = default;
	/// 	virtual int Do() = 0;
	/// };
	/// 
	/// struct CInterface : public virtual IInterface
	/// {
	/// 	CInterface() = default;
	/// 	virtual ~CInterface() = default;
	/// 	virtual int Do() final override { return 1111; }
	/// };
	/// 
	/// 
	/// struct IParamTest
	/// {
	/// 	virtual ~IParamTest() = default;
	/// 	virtual void Run(_In_ Extensions::ParameterPack&& oPack) = 0;
	/// };
	/// struct CParamTest : public virtual IParamTest
	/// {
	/// 	CParamTest() = default;
	/// 	virtual ~CParamTest() = default;
	/// 	virtual void Run(_In_ Extensions::ParameterPack&& oPack) override
	/// 	{
	/// 		int a = 0;
	/// 		int b = 0;
	/// 		int *c = nullptr;
	/// 		std::shared_ptr<int> d = nullptr;
	/// 		CInterface* pInt = nullptr;
	/// 
	///			//Use unpack by output parameters
	/// 		oPack.GetPack(a, b, c, d, pInt);
	/// 
	///			//Use unpack by return tuple
	/// 		auto [iNumber1, iNumber2, pNumber, pShared, pInterface] = oPack.GetPack<int, int, int*, std::shared_ptr<int>, CInterface*>();
	/// 
	///		}
	/// };
	/// 
	/// 
	/// void SomeFunc()
	/// {
	///		CParamTest oTest;
	///		
	///		int* pInt = new int(666);
	///		auto pShared = std::make_shared<int>(777);
	///		CInterface oInt;
	///		
	///		// Initialize parameter pack
	///		auto oPack = Extensions::ParameterPack(25, 333, pInt, pShared, &oInt);
	///		oTest.Run(std::move(oPack));
	/// }
	/// </code>
	/// </example>
	class ParameterPack
	{
	public:
		using Parameters = std::list<std::shared_ptr<ParameterBase>>;
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
			_Inout_ Args& ... oArgs) const
		{

			if (m_listArgs.size() != sizeof...(Args))
				return;

			auto listArgs = m_listArgs;
			_DeserializeParameters(std::move(listArgs), oArgs...);
		}

		template <class ... Args>
		std::tuple<Args...> GetPack()
		{
			std::tuple<Args...> oTuple = {};
			if (m_listArgs.size() != sizeof...(Args))
				return oTuple;

			auto listArgs = m_listArgs;
			return _DeserializeParametersTuple<Args...>(std::move(listArgs));
		}

	protected:
		template <class T>
		void _SerializeParameters(
			_In_ const T& oFirst)
		{
			static_assert(!is_unique_ptr<T>::value, "Cannot save unique_ptr<T>, because resource ownership might be broken!");
			m_listArgs.emplace_back(std::make_shared<Parameter<T>>(oFirst));
		}

		template <class T, class... Rest>
		void _SerializeParameters(
			_In_ const T& oFirst,
			_In_ const Rest& ... oRest)
		{
			_SerializeParameters(oFirst);
			_SerializeParameters(oRest...);
		}

		template <class T>
		void _DeserializeParameters(
			_In_ Parameters&& listArgs,
			_Inout_ T& oFirst) const
		{
			static_assert(!is_unique_ptr<T>::value, "Cannot load unique_ptr<T>, because resource ownership might be broken!");
			oFirst = listArgs.front()->Get<T>();
			listArgs.pop_front();
		}

		template <class T, class... Rest>
		void _DeserializeParameters(
			_In_ Parameters&& listArgs,
			_Inout_ T& oFirst,
			_Inout_ Rest& ... oRest) const
		{
			_DeserializeParameters(std::move(listArgs), oFirst);
			_DeserializeParameters(std::move(listArgs), oRest...);
		}

		template <class T, class... Rest>
		std::tuple<T, Rest...> _DeserializeParametersTuple(
			_In_ Parameters&& listArgs) const
		{

			auto oTuple = std::make_tuple(listArgs.front()->Get<T>());
			listArgs.pop_front();

			if constexpr (sizeof...(Rest) > 0)
				return std::tuple_cat(oTuple, _DeserializeParametersTuple<Rest...>(std::move(listArgs)));
			
			return std::tuple_cat(oTuple, std::tuple<Rest...>());
		}

	protected:
		Parameters m_listArgs;
	};


	

} //namespace Extensions
