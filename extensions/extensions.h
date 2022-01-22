/*
Copyright (c) 2020 Vit janecek <mailto:janecekvit@outlook.com>.
All rights reserved.

extensions.h
Purpose:	header file contains set of extended methods implemented over stl containers

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.13 07/11/2019
*/

#pragma once
#include "extensions/concurrent.h"

#include <algorithm>
#include <any>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>

///Namespace owns set of extended methods implemented over stl containers
namespace janecekvit::extensions
{
/// <summary>
/// execute_on_container
/// Template method creates wrapper over each container that implements find method()
/// Method used input key to search value in container and calls callback with value as the parameter.
/// Template method deduce return type from callback's return type.
/// Implementation calls native find methods for this containers:
///		std::unordered_map, std::unordered_multimap, std::map and std::multimap returns iterator->second value, that CAN be modified by callback
///		std::set, std::unordered_set returns *iterator value, that CANNOT be modified by callback
/// Other containers use std::find() method and returns *iterator value
/// </summary>
/// <param name="container">The input container defined by begin() and end() iterators.</param>
/// <param name="key">The input key to search value in container.</param>
/// <param name="callback">The lambda/functor callback what is called when value was found in container.</param>
/// <returns>Deduced return type from callback's return type</returns>
/// <example>
/// <code>
///	std::unordered_map<int, int> mapInts;
///	mapInts.emplace(5, 10);
///	auto iResult = extensions::execute_on_container(mapInts, 5, [](int &oResult)
///	{
///		oResult += 1;
///		return oResult;
///	});
/// </code>
/// </example>
template <template <class...> class _Container, class _Key, class... _Args, class _Func>
constexpr decltype(auto) execute_on_container(
	_Container<_Args...>& container,
	const _Key& key,
	_Func&& callback)
{
	if constexpr (constraints::is_foundable_v<_Container<_Args...>, _Key>)
	{
		if constexpr (constraints::is_pair_v<std::_Iter_value_t<decltype(container.find(key))>>)
		{
			auto&& it = container.find(key);
			if (it != container.end())
				return callback(it->second);

			if constexpr (!std::is_void_v<decltype(callback(it->second))>)
				return decltype(callback(it->second)){};
		}
		else
		{
			auto&& it = container.find(key);
			if (it != container.end())
				return callback(*it);

			if constexpr (!std::is_void_v<decltype(callback(*it))>)
				return decltype(callback(*it)){};
		}
	}
	else
	{
		auto&& it = std::find(container.begin(), container.end(), key);
		if (it != container.end())
			return callback(*it);

		if constexpr (!std::is_void_v<decltype(callback(*it))>)
			return decltype(callback(*it)){};
	}
}

template <template <class...> class _Container, class _Key, class... _Args, class _Func>
constexpr decltype(auto) execute_on_container(
	const _Container<_Args...>& container,
	const _Key& key,
	_Func&& callback)
{
	return execute_on_container(const_cast<_Container<_Args...>&>(container), key, callback);
}

/// <summary>
/// for_each() method internally calls std::for_each method with lambda predicator over input collection
/// </summary>
/// <param name="container">The input container defined by begin() and end() iterators.</param>
/// <param name="callback">The lambda/functor callback called for each value in container.</param>
/// <returns>void()</returns>
template <template <class...> class _Container, class... _Args, class _Func>
constexpr auto for_each(
	_Container<_Args...>& container,
	_Func&& callback)
	-> decltype(std::begin(container), std::end(container), void())
{
	std::for_each(container.begin(), container.end(), callback);
}
template <template <class...> class _Container, class... _Args, class _Func>
constexpr auto for_each(
	const _Container<_Args...>& container,
	_Func&& callback)
	-> decltype(std::begin(container), std::end(container), void())
{
	for_each(const_cast<_Container<_Args...>&>(container), callback);
}

/// <summary>
/// any_of() method internally calls std::any_of method with  lambda predicator over input collection
/// <param name="container">The input container defined by begin() and end() iterators.</param>
/// <param name="callback">The lambda/functor callback called for each value in container.</param>
/// <returns>return true, when callback predicate found result for any item in container</returns>
/// </summary>
template <template <class...> class _Container, class... _Args, class _Func>
[[nodiscard]] constexpr auto any_of(
	_Container<_Args...>& container,
	_Func&& callback)
	-> decltype(std::begin(container), std::end(container), callback(*std::begin(container)), bool())
{
	return std::any_of(container.begin(), container.end(), callback);
}
template <template <class...> class _Container, class... _Args, class _Func>
[[nodiscard]] constexpr auto any_of(
	const _Container<_Args...>& container,
	_Func&& callback)
	-> decltype(std::begin(container), std::end(container), callback(*std::begin(container)), bool()) const
{
	return any_of(const_cast<_Container<_Args...>&>(container), callback);
}

/// <summary>
/// Method implements RAII memory wrapper recasting from _Base to TDervied
/// Method convert current instance of RAII memory wrapper to the new one
/// Method is atomically, if recast cannot be done, input pointer is still valid
/// </summary>
/// <returns>On success, returns recasted memory-safe pointer, else do not nothing</returns>
template <class _Base, class _TDerived>
[[nodiscard]] constexpr std::unique_ptr<_TDerived> recast(
	std::unique_ptr<_Base>& item)
{
	auto* pTemp = dynamic_cast<_TDerived*>(item.get());
	if (pTemp)
		return nullptr;

	item.release();
	return std::unique_ptr<_TDerived>(pTemp);
}

/// <summary>
/// Method implements RAII memory wrapper recasting from TDervied to _Base
/// Method convert current instance of RAII memory wrapper to the new one
/// Method is atomically, if recast cannot be done, input pointer is still valid
/// </summary>
/// <returns>On success, returns recasted memory-safe pointer, else do not nothing</returns>
template <class _Base, class _TDerived>
[[nodiscard]] constexpr std::unique_ptr<_Base> recast(
	std::unique_ptr<_TDerived>& item)
{
	auto* pTemp = dynamic_cast<_Base*>(item.get());
	if (pTemp)
		return nullptr;

	item.release();
	return std::unique_ptr<_Base>(pTemp);
}

namespace Storage
{
/// <summary>
/// parameter pack class can forward input variadic argument's list to the any object for future processing
/// parameter pack implement lazy evaluation idiom to enable processing input arguments as late as possible
/// Packed parameters can be retrieved from pack by out parameters or by return value through std::tuple
/// </summary>
/// <exception cref="std::invalid_argument">When bad number of arguments received in Get methods.</exception>
/// <example>
/// <code>
///
/*
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
/// 	virtual void Run(extensions::Storage::parameter_pack_legacy&& oPack) = 0;
/// };
/// struct CParamTest : public virtual IParamTest
/// {
/// 	CParamTest() = default;
/// 	virtual ~CParamTest() = default;
/// 	virtual void Run(extensions::Storage::parameter_pack_legacy&& oPack) override
/// 	{
/// 		int a = 0;
/// 		int b = 0;
/// 		int *c = nullptr;
/// 		std::shared_ptr<int> d = nullptr;
/// 		CInterface* pInt = nullptr;
/// 
///			//Use unpack by output parameters
/// 		oPack.get_pack(a, b, c, d, pInt);
/// 
///			//Use unpack by return tuple
/// 		auto [iNumber1, iNumber2, pNumber, pShared, pInterface] = oPack.get_pack<int, int, int*, std::shared_ptr<int>, CInterface*>();
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
///		auto oPack = extensions::parameter_pack(25, 333, pInt, pShared, &oInt);
///		oTest.Run(std::move(oPack));
/// }
/// 
/// 
*/
/// </code>
/// </example>
class parameter_pack_legacy
{
private:
	// Helper base class used as wrapper to hold any type described by derived parameter<_T> class
	class parameter_base
	{
	public:
		virtual ~parameter_base() = default;

		template <class _T>
		const _T& get() const; // Method is implemented after parameter derived class, to gain ability to operate with it
	};

	// Helper derived class used as container of any input type.
	template <class _T>
	class parameter : public virtual parameter_base
	{
	public:
		virtual ~parameter() = default;

		parameter(const _T& oValue)
			: m_oValue(oValue)
		{
		}

		constexpr const _T& get() const
		{
			return m_oValue;
		}

	protected:
		_T m_oValue = {};
	};

	// Main class
public:
	using Parameters				 = std::list<std::shared_ptr<parameter_base>>;
	parameter_pack_legacy()			 = default;
	virtual ~parameter_pack_legacy() = default;

	template <class... _Args>
	parameter_pack_legacy(
		const _Args&... args)
	{
		_serialize(args...);
	}

public:
	template <class... _Args>
	constexpr void get_pack(
		_Args&... args) const
	{
		if (_arguments.size() != sizeof...(_Args))
			throw std::invalid_argument("Bad number of input arguments!");

		auto tempArgs = _arguments;
		_deserialize(std::move(tempArgs), args...);
	}

	template <class... _Args>
	[[nodiscard]] constexpr std::tuple<_Args...> get_pack() const
	{
		if (_arguments.size() != sizeof...(_Args))
			throw std::invalid_argument("Bad number of input arguments!");

		auto args = _arguments;
		return _DeserializeTuple<_Args...>(std::move(args));
	}

protected:
	template <class _T>
	constexpr void _serialize(
		const _T& first)
	{
		static_assert(std::is_copy_constructible<_T>::value, "Cannot assign <_T> type, because isn't CopyConstructible!");
		_arguments.emplace_back(std::make_shared<parameter<_T>>(first));
	}

	template <class _T, class... _Rest>
	constexpr void _serialize(
		const _T& first,
		const _Rest&... rest)
	{
		_serialize(first);
		_serialize(rest...);
	}

	template <class _T>
	constexpr void _deserialize(
		Parameters&& args,
		_T& first) const
	{
		static_assert(std::is_copy_constructible<_T>::value, "Cannot assign <_T> type, because isn't CopyConstructible!");
		first = args.front()->get<_T>();
		args.pop_front();
	}

	template <class _T, class... _Rest>
	constexpr void _deserialize(
		Parameters&& args,
		_T& first,
		_Rest&... rest) const
	{
		_deserialize(std::move(args), first);
		_deserialize(std::move(args), rest...);
	}

	template <class _T, class... _Rest>
	[[nodiscard]] constexpr std::tuple<_T, _Rest...> _DeserializeTuple(
		Parameters&& args) const
	{
		auto oTuple = std::make_tuple(args.front()->get<_T>());
		args.pop_front();

		if constexpr (sizeof...(_Rest) > 0)
			return std::tuple_cat(oTuple, _DeserializeTuple<_Rest...>(std::move(args)));
		else
			return std::tuple_cat(oTuple, std::tuple<_Rest...>());
	}

protected:
	Parameters _arguments;
};

// Core method: create dynamic_cast instead of virtual cast to get type what allocated in derived class
template <class _T>
const _T& Storage::parameter_pack_legacy::parameter_base::get() const
{
	using TRetrievedType = typename std::remove_cv<typename std::remove_reference<decltype(std::declval<parameter<_T>>().get())>::type>::type;
	static_assert(std::is_same<_T, TRetrievedType>::value, "Cannot cast templated return type <_T> to the derived class \"parameter.<_T>Get()\" type!");

	return dynamic_cast<const parameter<_T>&>(*this).get();
}

/// <summary>
/// parameter pack class can forward input variadic argument's list to the any object for future processing
/// parameter pack implement lazy evaluation idiom to enable processing input arguments as late as possible
/// Packed parameters can be retrieved from pack by return value through std::tuple
/// Second version of parameter pack, the Pack2 is recommended for version C++17 and above.
/// </summary>
/// <exception cref="std::invalid_argument">When bad number of arguments received in Get methods.</exception>
/// <example>
/// <code>
///
/*
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
/// 	virtual void Run(extensions::Storage::parameter_pack&& oPack) = 0;
/// };
/// struct CParamTest : public virtual IParamTest
/// {
/// 	CParamTest() = default;
/// 	virtual ~CParamTest() = default;
/// 	virtual void Run(extensions::Storage::ParemeterPack22&& oPack) override
/// 	{
/// 		auto [iNumber1, iNumber2, pNumber, pShared, pInterface] = oPack.get_pack<int, int, int*, std::shared_ptr<int>, CInterface*>();
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
///		auto oPack = extensions::parameter_pack(25, 333, pInt, pShared, &oInt);
///		oTest.Run(std::move(oPack));
/// }
/// 
*/
///
/// </code>
/// </example>
class parameter_pack
{
public:
	using Parameters = std::list<std::any>;

	parameter_pack()		  = default;
	virtual ~parameter_pack() = default;

	template <class... _Args>
	constexpr parameter_pack(
		const _Args&... args)
	{
		_serialize(args...);
	}

	template <class... _Args>
	[[nodiscard]] constexpr std::tuple<_Args...> get_pack() const
	{
		if (_arguments.size() != sizeof...(_Args))
			throw std::invalid_argument("Bad number of input arguments!");

		auto args = _arguments;
		return _deserialize<_Args...>(std::move(args));
	}

	[[nodiscard]] size_t size() const noexcept
	{
		return _arguments.size();
	}

protected:
	template <class _T, class... _Rest>
	[[nodiscard]] constexpr void _serialize(
		const _T& first,
		const _Rest&... rest)
	{
		static_assert(std::is_copy_constructible<_T>::value, "Cannot assign <_T> type, because isn't CopyConstructible!");
		_arguments.emplace_back(std::make_any<_T>(first));

		if constexpr (sizeof...(_Rest) > 0)
			_serialize(rest...);
	}

	template <class _T, class... _Rest>
	[[nodiscard]] constexpr std::tuple<_T, _Rest...> _deserialize(
		Parameters&& args) const
	{
		try
		{
			auto oValue = std::any_cast<_T>(args.front());
			auto oTuple = std::make_tuple(oValue);
			args.pop_front();

			if constexpr (sizeof...(_Rest) > 0)
				return std::tuple_cat(oTuple, _deserialize<_Rest...>(std::move(args)));

			return std::tuple_cat(oTuple, std::tuple<_Rest...>());
		}
		catch (const std::bad_any_cast& ex)
		{
			using namespace std::string_literals;
			throw std::invalid_argument("Wrong input type "s + ex.what() + "!");
		}
	}

protected:
	Parameters _arguments;
};

/// <summary>
/// Heterogeneous Container store any copy constructible object for the future processing
///  Heterogeneous Container implement lazy evaluation idiom to enable processing input arguments as late as possible
/// <exception cref="heterogeneous_container_exception">When cast to the input type failed.</exception>
/// </summary>
class heterogeneous_container final
{
public:
	class heterogeneous_container_exception : public std::exception
	{
	public:
		heterogeneous_container_exception(const std::type_info& typeInfo, const std::string& sError) noexcept
		{
			using namespace std::string_literals;
			_text = "heterogeneous_container: "s + sError + " with specified type: "s + typeInfo.name();
		}

		heterogeneous_container_exception(const std::type_info& typeInfo, const std::bad_any_cast& ex) noexcept
		{
			using namespace std::string_literals;
			_text = "heterogeneous_container: "s + ex.what() + " to: "s + typeInfo.name();
		}

		~heterogeneous_container_exception() = default;

		const char* what() const override
		{
			return _text.c_str();
		}

	protected:
		std::string _text;
	};

public:
	~heterogeneous_container() = default;

	heterogeneous_container() noexcept = default;

	template <class... _Args>
	constexpr heterogeneous_container(
		const _Args&... args)
	{
		_serialize(args...);
	}

	template <class... _Args>
	constexpr void emplace(const _Args&... args)
	{
		_serialize(args...);
	}

	template <class _T>
	constexpr void reset()
	{
		auto oScope = m_umapArgs.exclusive();
		execute_on_container(oScope.get(), std::type_index(typeid(_T)), [](std::list<std::any>& listInput)
			{
				listInput.clear();
			});
	}

	void reset()
	{
		m_umapArgs.exclusive()->clear();
	}

public:
	template <class _Func, class... _Args, std::enable_if_t<std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>, int> = 0>
	constexpr decltype(auto) call_first(_Args&&... args) const
	{
		auto&& oFunc = first<_Func>();
		return std::invoke(oFunc, std::forward<_Args>(args)...); //NVRO
	}

	template <class _Func, class... _Args, std::enable_if_t<std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>, int> = 0>
	constexpr decltype(auto) call(size_t uPosition, _Args&&... args) const
	{
		auto&& oFunc = get<_Func>(uPosition);
		return std::invoke(oFunc, std::forward<_Args>(args)...); //NVRO
	}

	template <class _Func, class... _Args, std::enable_if_t<std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>, int> = 0>
	constexpr decltype(auto) call_all(_Args&&... args) const
	{
		using RetType = std::invoke_result_t<_Func, _Args...>;
		if constexpr (std::is_void_v<RetType>)
		{
			for (auto&& func : get<_Func>())
				std::invoke(func, std::forward<_Args>(args)...);
		}
		else
		{
			std::list<RetType> oList = {};
			for (auto&& func : get<_Func>())
				oList.emplace_back(std::invoke(func, std::forward<_Args>(args)...));
			return oList;
		}
	}

	template <class _T>
	[[nodiscard]] constexpr size_t size() const noexcept
	{
		auto oScope = m_umapArgs.concurrent();
		return execute_on_container(oScope.get(), std::type_index(typeid(_T)), [](std::list<std::any>& listInput)
			{
				return listInput.size();
			});
	}

	template <class _T>
	[[nodiscard]] constexpr bool contains() const noexcept
	{
		return size<_T>() > 0;
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) get() const
	{
		return _deserialize<_T>();
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) get(size_t uPosition) const
	{
		return _deserialize<_T>(uPosition);
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) first() const
	{
		return _deserialize<_T>(0);
	}

	template <class _T>
	constexpr void visit(std::function<void(const _T&)>&& fnCallback) const
	{
		_visit<_T>(std::move(fnCallback));
	}

	template <class _T>
	constexpr void visit(std::function<void(_T&)>&& fnCallback)
	{
		_visit<_T>(std::move(fnCallback));
	}

private:
	template <class _T, class... _Rest>
	constexpr void _serialize(
		const _T& first,
		const _Rest&... rest)
	{
		static_assert(std::is_copy_constructible<_T>::value, "Cannot assign <_T> type, because isn't CopyConstructible!");
		m_umapArgs.exclusive()[std::type_index(typeid(_T))].emplace_back(std::make_any<_T>(first));

		if constexpr (sizeof...(_Rest) > 0)
			_serialize(rest...);
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) _deserialize() const
	{
		std::list<_T> oList = {};
		_visit<_T>([&oList](const _T& input)
			{
				oList.emplace_back(input);
			});
		return oList;
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) _deserialize(size_t uPosition) const
	{
		size_t uCounter			 = 0;
		std::optional<_T> oValue = std::nullopt;
		_visit<_T>([&](const _T& input)
			{
				if (uCounter == uPosition)
					oValue = std::make_optional<_T>(input);
				uCounter++;
			});

		if (!oValue)
			throw heterogeneous_container_exception(typeid(_T), "Cannot retrieve value on position " + std::to_string(uPosition));
		return static_cast<_T>(std::move(oValue.value()));
	}

	template <class _T>
	constexpr void _visit(std::function<void(_T&)>&& fnCallback) const
	{
		try
		{
			auto oScope = m_umapArgs.concurrent();
			execute_on_container(oScope.get(), std::type_index(typeid(_T)), [&fnCallback](std::list<std::any>& listInput)
				{
					for (auto&& item : listInput)
						fnCallback(std::any_cast<_T&>(item));
				});
		}
		catch (const std::bad_any_cast& ex)
		{
			throw heterogeneous_container_exception(typeid(_T), ex);
		};
	}

protected:
	concurrent::unordered_map<std::type_index, std::list<std::any>> m_umapArgs;
};

} //namespace Storage

namespace tuple
{
namespace details
{
template <typename _F, size_t... _Is>
constexpr auto generate(_F func, std::index_sequence<_Is...>)
{
	return std::make_tuple(func(_Is)...);
}

template <class _Tuple, std::size_t... _I>
constexpr Storage::heterogeneous_container unpack(_Tuple&& t, std::index_sequence<_I...>)
{
	return Storage::heterogeneous_container{ std::get<_I>(t)... };
}

} // namespace details

/// <summary>
/// generate sequence of integers from the input size N
/// </summary>
/// <example>
/// <code>
///
/// auto fnCallback = [](auto&&... args) -> int
/// {
///		auto tt = std::forward_as_tuple(args...);
///		return std::get<0>(tt);
/// };
/// auto oResultGenerator = extensions::_Tuple::generate<10>(fnCallback);
/// </code>
/// </example>
template <char _N, typename _F>
[[nodiscard]] constexpr decltype(auto) generate(_F func)
{
	return details::generate(func, std::make_index_sequence<_N>{});
}

/// <summary>
/// unpack tuple to the Heterogeneous container
/// </summary>
template <class _Tuple>
[[nodiscard]] constexpr Storage::heterogeneous_container unpack(_Tuple&& t)
{
	return details::unpack(std::forward<_Tuple>(t), std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<_Tuple>>>{});
}

template <class _Stream, class... _Args>
auto& operator<<(_Stream& os, const std::tuple<_Args...>& t)
{
	std::apply([&os](auto&&... args)
		{
			((os << args), ...);
		},
		t);
	return os;
}
template <class... _Args>
std::stringstream print(const std::tuple<_Args...>& t, const std::string& sDelimiter)
{
	std::stringstream ssStream;
	std::apply([&ssStream, &sDelimiter](auto&&... args)
		{
			((ssStream << args << sDelimiter), ...);
		},
		t);
	return ssStream;
}

} // namespace tuple

namespace numeric
{
template <size_t _N>
struct factorial
{
	static constexpr size_t value = _N * factorial<_N - 1>::value;
};

template <>
struct factorial<0>
{
	static constexpr size_t value = 1;
};

} // namespace numeric

/// <summary>
/// Hash compute mechanism used to provide unique hash from set of input values
/// </summary>
namespace hash
{
template <class _T>
constexpr size_t combine(const _T& oValue)
{
	return std::hash<_T>{}(oValue);
}

template <class _T, class... _Args>
constexpr size_t combine(const _T& oValue, const _Args&... args)
{
	size_t uSeed = combine(args...);
	uSeed ^= std::hash<_T>{}(oValue) + 0x9e3779b9 + (uSeed << 6) + (uSeed >> 2);
	return uSeed;
}

} // namespace hash

} // namespace janecekvit::extensions
