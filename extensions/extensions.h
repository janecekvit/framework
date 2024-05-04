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

namespace tuple
{

namespace details
{
template <typename _F, size_t... _Is>
constexpr auto generate(_F func, std::index_sequence<_Is...>)
{
	return std::make_tuple(func(_Is)...);
}

//template <class _Tuple, std::size_t... _I>
//constexpr storage::heterogeneous_container unpack(_Tuple&& t, std::index_sequence<_I...>)
//{
//	return storage::heterogeneous_container{ std::get<_I>(t)... };
//}

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

///// <summary>
///// unpack tuple to the Heterogeneous container
///// </summary>
//template <class _Tuple>
//[[nodiscard]] constexpr storage::heterogeneous_container unpack(_Tuple&& t)
//{
//	return details::unpack(std::forward<_Tuple>(t), std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<_Tuple>>>{});
//}

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
