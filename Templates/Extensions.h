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
@version 1.09 17/03/2019
*/

#pragma once
#include <any>
#include <list>
#include <tuple>
#include <memory>
#include <functional>
#include <type_traits>
#include <typeindex>

///Namespace owns set of extended methods implemented over stl containers
namespace Extensions
{
/// <summary>
/// Helper structures to determine if template type <T> is std::shared_ptr
/// </summary>
template<class T> struct is_shared_ptr_helper : std::false_type
{
};
template<class T> struct is_shared_ptr_helper<std::shared_ptr<T>> : std::true_type
{
};
template<class T> struct is_shared_ptr : is_shared_ptr_helper<typename std::remove_cv<T>::type>
{
};
template <class T> constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

/// <summary>
/// Helper structures to determine if template type <T> is std::unique_ptr
/// </summary>
template<class T> struct is_unique_ptr_helper : std::false_type
{
};
template<class T> struct is_unique_ptr_helper<std::unique_ptr<T>> : std::true_type
{
};
template<class T> struct is_unique_ptr : is_unique_ptr_helper<typename std::remove_cv<T>::type>
{
};
template <class T> constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

/// <summary>
/// Helper structures to determine if template type <T> is std::pair
/// </summary>
template<class ... Args> struct is_pair_helper : std::false_type
{
};
template<class ... Args> struct is_pair_helper<std::pair<Args...>> : std::true_type
{
};
template<class ... Args> struct is_pair : is_pair_helper<typename std::remove_cv<Args...>::type>
{
};
template <class T> constexpr bool is_pair_v = is_pair<T>::value;

/// <summary>
/// Helper structures to determine if template type <T> thats is container with associative find
/// </summary>
template<class T, class U, class = void> struct is_foundable_helper : std::false_type
{
};
template<class T, class U> struct is_foundable_helper<T, U, std::void_t<decltype(std::declval<T>().find(std::declval<U>()))>> : std::true_type
{
};
template<class T, class U> struct is_foundable : is_foundable_helper<typename std::remove_cv<T>::type, typename std::remove_cv<U>::type>
{
};
template <class T, class U> constexpr bool is_foundable_v = is_foundable<T, U>::value;

/// <summary>
/// ContainerFindCallback() 
/// Template method creates wrapper over each container that implements find method()
/// Method used input key to search value in container and calls callback with value as the parameter.
/// Template method deduce return type from callback's return type. 
/// Implementation calls native find methods for this containers:
///		std::unordered_map, std::unordered_multimap, std::map and std::multimap returns iterator->second value, that CAN be modified by callback
///		std::set, std::unordered_set returns *iterator value, that CANNOT be modified by callback
/// Other containers use std::find() method and returns *iterator value
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
{
	if constexpr (is_foundable_v<Container<Args...>, Key>)
	{
		if constexpr (is_pair_v<std::iterator_traits<decltype(oContainer.find(oKey))>::value_type>)
		{
			auto&& it = oContainer.find(oKey);
			if (it != oContainer.end())
				return oCallback(it->second);

			if constexpr (!std::is_void_v<decltype(oCallback(it->second))>)
				return decltype(oCallback(it->second)){};
		}
		else
		{
			auto&& it = oContainer.find(oKey);
			if (it != oContainer.end())
				return oCallback(*it);

			if constexpr (!std::is_void_v<decltype(oCallback(*it))>)
				return decltype(oCallback(*it)){};
		}
	}
	else
	{
		auto &&it = std::find(oContainer.begin(), oContainer.end(), oKey);
		if (it != oContainer.end())
			return oCallback(*it);

		if constexpr (!std::is_void_v<decltype(oCallback(*it))>)
			return decltype(oCallback(*it)){};
	}

}

template <template <class ...> class Container, class Key, class ... Args, class Functor>
decltype(auto) ContainerFind(
	_In_ const Container<Args...>& oContainer,
	_In_ const Key& oKey,
	_In_ Functor&& oCallback)
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
	_In_ const Container<Args...>& oContainer,
	_In_ Functor&& oCallback)
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

//decltyp
//ForwardFromTuple


namespace Storage
{

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
/// 	virtual void Run(_In_ Extensions::Storage::ParameterPackLegacy&& oPack) = 0;
/// };
/// struct CParamTest : public virtual IParamTest
/// {
/// 	CParamTest() = default;
/// 	virtual ~CParamTest() = default;
/// 	virtual void Run(_In_ Extensions::Storage::ParameterPackLegacy&& oPack) override
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
///			//TODO: ANY STUFF
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
class ParameterPackLegacy
{
private:
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

	// Main class
public:
	using Parameters = std::list<std::shared_ptr<ParameterBase>>;
	virtual ~ParameterPackLegacy() = default;

	template <class ... Args>
	ParameterPackLegacy(
		_In_ const Args& ... oArgs)
	{
		_Serialize(oArgs...);
	}

public:
	template <class ... Args>
	void GetPack(
		_Inout_ Args& ... oArgs) const
	{
		if (m_listArgs.size() != sizeof...(Args))
			throw std::invalid_argument("Bad number of input arguments!");

		auto listArgs = m_listArgs;
		_Deserialize(std::move(listArgs), oArgs...);
	}

	template <class ... Args>
	std::tuple<Args...> GetPack()
	{
		if (m_listArgs.size() != sizeof...(Args))
			throw std::invalid_argument("Bad number of input arguments!");

		auto listArgs = m_listArgs;
		return _DeserializeTuple<Args...>(std::move(listArgs));
	}

protected:
	template <class T>
	void _Serialize(
		_In_ const T& oFirst)
	{
		static_assert(!is_unique_ptr_v<T>, "Cannot load unique_ptr<T>, because resource isn't CopyConstructible!");
		m_listArgs.emplace_back(std::make_shared<Parameter<T>>(oFirst));
	}

	template <class T, class... Rest>
	void _Serialize(
		_In_ const T& oFirst,
		_In_ const Rest& ... oRest)
	{
		_Serialize(oFirst);
		_Serialize(oRest...);
	}

	template <class T>
	void _Deserialize(
		_In_ Parameters&& listArgs,
		_Inout_ T& oFirst) const
	{
		static_assert(!is_unique_ptr_v<T>, "Cannot load unique_ptr<T>, because resource isn't CopyConstructible!");
		oFirst = listArgs.front()->Get<T>();
		listArgs.pop_front();
	}

	template <class T, class... Rest>
	void _Deserialize(
		_In_ Parameters&& listArgs,
		_Inout_ T& oFirst,
		_Inout_ Rest& ... oRest) const
	{
		_Deserialize(std::move(listArgs), oFirst);
		_Deserialize(std::move(listArgs), oRest...);
	}

	template <class T, class... Rest>
	std::tuple<T, Rest...> _DeserializeTuple(
		_In_ Parameters&& listArgs) const
	{

		auto oTuple = std::make_tuple(listArgs.front()->Get<T>());
		listArgs.pop_front();

		if constexpr (sizeof...(Rest) > 0)
			return std::tuple_cat(oTuple, _DeserializeTuple<Rest...>(std::move(listArgs)));

		return std::tuple_cat(oTuple, std::tuple<Rest...>());
	}

protected:
	Parameters m_listArgs;
};

// Core method: create dynamic_cast instead of virtual cast to get type what allocated in derived class
template<class T>
const T& Storage::ParameterPackLegacy::ParameterBase::Get() const
{
	using TRetrievedType = typename std::remove_cv<typename std::remove_reference<decltype(std::declval<Parameter<T>>().Get())>::type>::type;
	static_assert(std::is_same<T, TRetrievedType>::value, "Cannot cast templated return type <T> to the derived class \"Parameter.<T>Get()\" type!");

	return dynamic_cast<const Parameter<T>&>(*this).Get();
}

/// <summary>
/// Parameter pack class can forward input variadic argument's list to the any object for future processing
/// Parameter pack implement lazy evaluation idiom to enable processing input arguments as late as possible
/// Packed parameters can be retrieved from pack by return value through std::tuple 
/// Second version of Parameter pack, the Pack2 is recommended for version C++17 and above.
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
/// 	virtual void Run(_In_ Extensions::Storage::ParameterPack&& oPack) = 0;
/// };
/// struct CParamTest : public virtual IParamTest
/// {
/// 	CParamTest() = default;
/// 	virtual ~CParamTest() = default;
/// 	virtual void Run(_In_ Extensions::Storage::ParemeterPack22&& oPack) override
/// 	{
/// 		auto [iNumber1, iNumber2, pNumber, pShared, pInterface] = oPack.GetPack<int, int, int*, std::shared_ptr<int>, CInterface*>();
/// 
///			//TODO: ANY STUFF
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
	using Parameters = std::list<std::any>;
	virtual ~ParameterPack() = default;

	template <class ... Args>
	ParameterPack(
		_In_ const Args& ... oArgs)
	{
		_Serialize(oArgs...);
	}
	
	//template <class ... Args>
	//void Emplace(_In_ const Args& ... oArgs)
	//{
	//	_Serialize(oArgs...);
	//}

	template <class ... Args>
	std::tuple<Args...> GetPack()
	{
		std::tuple<Args...> oTuple = {};
		if (m_listArgs.size() != sizeof...(Args))
			throw std::invalid_argument("Bad number of input arguments!");

		auto listArgs = m_listArgs;
		return _Deserialize<Args...>(std::move(listArgs));
	}

	size_t Size()
	{
		return m_listArgs.size();
	}

protected:
	template <class T, class... Rest>
	void _Serialize(
		_In_ const T& oFirst,
		_In_ const Rest& ... oRest)
	{
		static_assert(!is_unique_ptr_v<T>, "Cannot load unique_ptr<T>, because resource isn't CopyConstructible!");
		m_listArgs.emplace_back(std::make_any<T>(oFirst));
		
		if constexpr (sizeof...(Rest) > 0)
			_Serialize(oRest...);
	}

	template <class T, class... Rest>
	std::tuple<T, Rest...> _Deserialize(
		_In_ Parameters&& listArgs) const
	{

		try
		{
			auto oValue = std::any_cast<T>(listArgs.front());
			using TRetrievedType = typename std::remove_cv<typename std::remove_reference<decltype(oValue)>::type>::type;
			static_assert(std::is_same<T, TRetrievedType>::value, "Cannot recast return type <T> to the derived class \"std::any_cast<T>(listArgs.front()\" type!");

			auto oTuple = std::make_tuple(oValue);
			listArgs.pop_front();

			if constexpr (sizeof...(Rest) > 0)
				return std::tuple_cat(oTuple, _Deserialize<Rest...>(std::move(listArgs)));

			return std::tuple_cat(oTuple, std::tuple<Rest...>());
		}
		catch (const std::bad_any_cast&)
		{
			return std::tuple<T, Rest...>{};
		}
	}

protected:
	Parameters m_listArgs;
};


class HeterogeneousContainer
{
public:
	virtual ~HeterogeneousContainer() = default;

	/*template <class ... Args>
	HeterogeneousContainer(
		_In_ std::tuple<Args...>&& oArgs)
	{
		HeterogeneousContainer(std::forward<Args...>(oArgs));
	}*/

	template <class ... Args>
	HeterogeneousContainer(
		_In_ const Args& ... oArgs)
	{
		_Serialize(oArgs...);
	}

public:
	template <class T>//<class ... Args, class = std::common_type_t<Args...>>
	decltype(auto) Get() const
	{
		return _Deserialize<T>();
	}

protected:
	template <class T, class... Rest>
	void _Serialize(
		_In_ const T& oFirst,
		_In_ const Rest& ... oRest)
	{
		static_assert(!is_unique_ptr_v<T>, "Cannot load unique_ptr<T>, because resource isn't CopyConstructible!");
		m_umapArgs[std::type_index(typeid(T))].emplace_back(std::make_any<T>(oFirst));

		if constexpr (sizeof...(Rest) > 0)
			_Serialize(oRest...);
	}

	template  <class T>//, class ... Rest> //, class = std::common_type_t<Rest...>
	decltype(auto) _Deserialize() const
	{
		const auto&& it = m_umapArgs.find(std::type_index(typeid(T)));
		if (it == m_umapArgs.end())
			return std::tuple<>{};

		std::tuple<T> oTuple = {};
		for (auto item : it->second)
			oTuple = std::tuple_cat(oTuple, std::any_cast<T>(item));
		return oTuple;
	}

protected:
	std::unordered_map<std::type_index, std::list<std::any>> m_umapArgs;
};

} //namespace Storage


/// <summary>
/// Hash compute mechanism used to provide unique hash from set of input values
/// </summary>
namespace Hash
{

template <class T>
size_t Combine(_In_ const T oValue)
{
	return std::hash<T>{}(oValue);
}

template <class T, class ... Args>
size_t Combine(_In_ const T oValue, _In_ const Args ... oArgs)
{
	size_t uSeed = Combine(oArgs...);
	uSeed ^= std::hash<T>{}( oValue) +0x9e3779b9 + (uSeed << 6) + (uSeed >> 2);
	return uSeed;
}

} //namespace Hash

} //namespace Extensions
