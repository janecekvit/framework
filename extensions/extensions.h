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

#include "extensions/constraints.h"

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

/// Namespace owns set of extended methods implemented over stl containers
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
template <class _Container, class _Key, class _Func>
constexpr decltype(auto) execute_on_container(
	_Container& container,
	const _Key& key,
	_Func&& callback)
{
	if constexpr (constraints::is_foundable_v<_Container, _Key>)
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


template <class _Container, class _Key, class _Func>
	requires std::invocable<_Func, const typename _Container::value_type>
constexpr decltype(auto) execute_on_container(
	const _Container& container,
	const _Key& key,
	_Func&& callback)
{
	return execute_on_container(const_cast<_Container&>(container), key, callback);
}

/// <summary>
/// Method cast the _From type in std::unique_ptr to their inherited forms
/// </summary>
/// <typeparam name="_From">The type of the original pointer</typeparam>
/// <typeparam name="_To">The type of the target pointer</typeparam>
/// <param name="item">The unique pointer to be recasted</param>
/// <returns>returns the recasted memory-safe pointer</returns>
template <class _From, class _To>
[[nodiscard]] constexpr std::unique_ptr<_To> recast(std::unique_ptr<_From>&& item)
	requires (std::derived_from<_To, _From> || std::derived_from<_From, _To>) && (!std::is_same_v<_From, _To>)
{
	auto pTemp = dynamic_cast<_To*>(item.get());
	if (!pTemp)
		throw std::bad_cast{}; // throw exception if recast is not possible
	item.release();
	return std::unique_ptr<_To>(std::move(pTemp));
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
