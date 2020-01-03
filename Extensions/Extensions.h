/*
Copyright (c) 2019 Vit janecek <mailto:janecekvit@outlook.com>.
All rights reserved.

Extensions.h
Purpose:	header file contains set of extended methods implemented over stl containers

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.13 07/11/2019
*/

#pragma once
#include <any>
#include <list>
#include <tuple>
#include <memory>
#include <string>
#include <sstream>
#include <optional>
#include <algorithm>
#include <typeindex>
#include <functional>
#include <type_traits>

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
/// Helper structures to determine if template type <T> thats is container with associative find
/// </summary>
template<class T, class = void, class = void, class = void> struct is_container_helper : std::false_type
{
};
template<class T> struct is_container_helper<T, std::void_t<decltype(std::declval<T>().begin())>, std::void_t<decltype(std::declval<T>().end())>, std::void_t<decltype(std::declval<T>().size())>> : std::true_type
{
};
template<class T> struct is_container : is_container_helper<typename std::remove_cv<T>::type>
{
};
template <class T> constexpr bool is_container_v = is_container<T>::value;

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
decltype(auto) ContainerFind(
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

namespace Storage
{

/// <summary>
/// Parameter pack class can forward input variadic argument's list to the any object for future processing
/// Parameter pack implement lazy evaluation idiom to enable processing input arguments as late as possible
/// Packed parameters can be retrieved from pack by out parameters or by return value through std::tuple 
/// </summary>
/// <exception cref="std::invalid_argument">When bad number of arguments received in Get methods.</exception>
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
		static_assert(std::is_copy_constructible<T>::value, "Cannot assign <T> type, because isn't CopyConstructible!");
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
		static_assert(std::is_copy_constructible<T>::value, "Cannot assign <T> type, because isn't CopyConstructible!");
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
		else
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
/// <exception cref="std::invalid_argument">When bad number of arguments received in Get methods.</exception>
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
		static_assert(std::is_copy_constructible<T>::value, "Cannot assign <T> type, because isn't CopyConstructible!");
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

/// <summary>
/// Heterogeneous Container store any copy constructible object for the future processing
///  Heterogeneous Container implement lazy evaluation idiom to enable processing input arguments as late as possible
/// <exception cref="HeterogeneousContainerException">When cast to the input type failed.</exception>
/// </summary>
class HeterogeneousContainer final
{
public:
	class HeterogeneousContainerException : public std::exception
	{
	public:
		HeterogeneousContainerException(_In_ const std::type_info& typeInfo, _In_ const std::string& sError) noexcept
		{
			using namespace std::string_literals;
			m_sData = "HeterogeneousContainer: "s + sError + " with specified type: "s + typeInfo.name();
		}

		HeterogeneousContainerException(_In_ const std::type_info& typeInfo, _In_ const std::bad_any_cast& ex) noexcept
		{
			using namespace std::string_literals;
			m_sData = "HeterogeneousContainer: "s + ex.what() + " to: "s + typeInfo.name();
		}

		~HeterogeneousContainerException() = default;

		const char* what() const override
		{
			return m_sData.c_str();
		}

	protected:
		std::string m_sData;
	};

public:
	virtual ~HeterogeneousContainer() = default;
	HeterogeneousContainer() noexcept = default;

	template <class ... Args>
	constexpr HeterogeneousContainer(
		_In_ const Args& ... oArgs)
	{
		_Serialize(oArgs...);
	}
	
	template <class Func, class ... Args, std::enable_if_t<std::is_invocable_r_v<std::invoke_result_t<Func, Args...>, Func, Args...>, int> = 0>
	decltype(auto) CallFirst(_In_ Args&& ... oArgs)
	{
		auto&& oFunc = First<Func>();
		return std::invoke(oFunc, std::forward<Args...>(oArgs)...); //NVRO
	}

	template <class Func, class ... Args, std::enable_if_t<std::is_invocable_r_v<std::invoke_result_t<Func, Args...>, Func, Args...>, int> = 0>
	decltype(auto) Call(_In_ size_t uPosition, _In_ Args&& ... oArgs)
	{
		auto&& oFunc = Get<Func>(uPosition);
		return std::invoke(oFunc, std::forward<Args...>(oArgs)...); //NVRO
	}

	template <class Func, class ... Args, std::enable_if_t<std::is_invocable_r_v<std::invoke_result_t<Func, Args...>, Func, Args...>, int> = 0>
	decltype(auto) CallAll(_In_ Args&& ... oArgs)
	{
		using RetType = std::invoke_result_t<Func, Args...>;		
		if constexpr (std::is_void_v<RetType>)
		{
			for (auto&& func : Get<Func>())
				std::invoke(func, std::forward<Args...>(oArgs)...);
		}
		else
		{
			std::list<RetType> oList = {};
			for (auto&& func : Get<Func>())
				oList.emplace_back(std::invoke(func, std::forward<Args...>(oArgs)...));
			return oList;
		}
	}
	
	template <class ... Args>
	constexpr void Emplace(_In_ const Args& ... oArgs)
	{
		_Serialize(oArgs...);
	}

	template <class T>
	constexpr decltype(auto) Get() const
	{
		return _Deserialize<T>();
	}

	template <class T>
	constexpr decltype(auto) Get(_In_ size_t uPosition) const
	{
		return _Deserialize<T>(uPosition);
	}

	template <class T>
	constexpr decltype(auto) First() const
	{
		return _Deserialize<T>(0);
	}

	template <class T>
	constexpr void Visit(_In_ std::function<void(const T&)>&& fnCallback) const
	{
		_Visit<T>(std::move(fnCallback));
	}

	template <class T>
	constexpr void Visit(_In_ std::function<void(T&)>&& fnCallback)
	{
		_Visit<T>(std::move(fnCallback));
	}

protected:
	template <class T, class... Rest>
	constexpr void _Serialize(
		_In_ const T& oFirst,
		_In_ const Rest& ... oRest)
	{
		static_assert(std::is_copy_constructible<T>::value, "Cannot assign <T> type, because isn't CopyConstructible!");
		m_umapArgs[std::type_index(typeid(T))].emplace_back(std::make_any<T>(oFirst));

		if constexpr (sizeof...(Rest) > 0)
			_Serialize(oRest...);
	}

	template  <class T>
	constexpr decltype(auto) _Deserialize() const
	{
		std::list<T> oList = {};
		_Visit<T>([&oList](const T& input)
		{
			oList.emplace_back(input);
		});
		return oList;
	}

	template  <class T>
	constexpr decltype(auto) _Deserialize(size_t uPosition) const
	{
		size_t uCounter = 0;
		std::optional<T> oValue = std::nullopt;
		_Visit<T>([&](const T& input)
		{
			if (uCounter == uPosition)
				oValue = std::make_optional<T>(input);
			uCounter++;
		});

		if (!oValue)
			throw HeterogeneousContainerException(typeid(T), "Cannot retrieve value on position " + std::to_string(uPosition));
		return static_cast<T>(std::move(oValue.value()));
	}
	
	template  <class T>
	constexpr void _Visit(_In_ std::function<void(T&)>&& fnCallback) const
	{
		try
		{
			ContainerFind(m_umapArgs, std::type_index(typeid(T)), [&fnCallback](std::list<std::any>& listInput)
			{
				for (auto&& item : listInput)
					fnCallback(std::any_cast<T&>(item));
			});
		}
		catch (const std::bad_any_cast & ex)
		{
			throw HeterogeneousContainerException(typeid(T), ex);
		};
	}

protected:
	std::unordered_map<std::type_index, std::list<std::any>> m_umapArgs;
};

} //namespace Storage

namespace Tuple
{

namespace Details
{
template <typename F, size_t... Is>
auto Generate(F func, std::index_sequence<Is...>)
{
	return std::make_tuple(func(Is)...);
}


template <class Tuple, std::size_t ...I>
constexpr Storage::HeterogeneousContainer Unpack(Tuple&& t, std::index_sequence<I...>)
{
	return Storage::HeterogeneousContainer { std::get<I>(t)... };
}

} //namespace Implementation


/// <summary>
/// Generate sequence of integers from the input size N 
/// </summary>
/// <example>
/// <code>
/// 
/// auto fnCallback = [](auto&&... oArgs) -> int
/// {
///		auto tt = std::forward_as_tuple(oArgs...);
///		return std::get<0>(tt);
/// };
/// auto oResultGenerator = Extensions::Tuple::Generate<10>(fnCallback);
/// </code>
/// </example>
template <size_t N, typename F>
decltype(auto) Generate(F func)
{
	return Details::Generate(func, std::make_index_sequence<N>{});
}

/// <summary>
/// Unpack tuple to the Heterogeneous container
/// </summary>
template <class Tuple>
constexpr Storage::HeterogeneousContainer Unpack(Tuple& t)
{
	return Details::Unpack(std::forward<Tuple>(t), std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
}

template <class TStream, class ... Args>
auto& operator<<(TStream& os, const std::tuple<Args...>& t)
{
	std::apply([&os](auto&&... oArgs)
	{
		((os << oArgs), ...);
	}, t);
	return os;
}
template<class ... Args >
std::stringstream Print(const std::tuple<Args...>& t, const std::string& sDelimiter)
{
	std::stringstream ssStream;
	std::apply([&ssStream, &sDelimiter](auto&&... oArgs)
	{
		((ssStream << oArgs << sDelimiter), ...);
	}, t);
	return ssStream;
}

} // namespace Tuple


namespace Numeric
{
template <size_t N>
struct factorial
{
	static constexpr size_t value = N * factorial<N - 1>::value;
};

template <>
struct factorial<0>
{
	static constexpr size_t value = 1;
};

} //namespace Numeric

/// <summary>
/// Hash compute mechanism used to provide unique hash from set of input values
/// </summary>
namespace Hash
{

template <class T>
size_t Combine(_In_ const T& oValue)
{
	return std::hash<T>{}(oValue);
}

template <class T, class ... Args>
size_t Combine(_In_ const T& oValue, _In_ const Args& ... oArgs)
{
	size_t uSeed = Combine(oArgs...);
	uSeed ^= std::hash<T>{}( oValue) +0x9e3779b9 + (uSeed << 6) + (uSeed >> 2);
	return uSeed;
}

} //namespace Hash

} //namespace Extensions
