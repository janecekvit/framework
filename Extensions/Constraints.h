/*
Copyright (c) 2020 Vit janecek <mailto:janecekvit@outlook.com>.
All rights reserved.

Constraints.h
Purpose:	header file contains set of extended constraints to describe stl containers

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.16 15/10/2020
*/

#pragma once
#include <algorithm>
#include <any>
#include <concepts>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>

///Namespace owns set of extended Constraints to describe stl containers
namespace Constraints
{
/// <summary>
/// Helper structures to determine if template type <T> is std::shared_ptr
/// </summary>
template <class T>
struct is_shared_ptr_helper : std::false_type
{
};
template <class T>
struct is_shared_ptr_helper<std::shared_ptr<T>> : std::true_type
{
};
template <class T>
struct is_shared_ptr : is_shared_ptr_helper<typename std::remove_cv<T>::type>
{
};
template <class T>
constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

/// <summary>
/// Helper structures to determine if template type <T> is std::unique_ptr
/// </summary>
template <class T>
struct is_unique_ptr_helper : std::false_type
{
};
template <class T>
struct is_unique_ptr_helper<std::unique_ptr<T>> : std::true_type
{
};
template <class T>
struct is_unique_ptr : is_unique_ptr_helper<typename std::remove_cv<T>::type>
{
};
template <class T>
constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

/// <summary>
/// Helper structures to determine if template type <T> is std::pair
/// </summary>
template <class... Args>
struct is_pair_helper : std::false_type
{
};
template <class... Args>
struct is_pair_helper<std::pair<Args...>> : std::true_type
{
};
template <class... Args>
struct is_pair : is_pair_helper<typename std::remove_cv<Args...>::type>
{
};
template <class T>
constexpr bool is_pair_v = is_pair<T>::value;

/// <summary>
/// Helper structures to determine if template type <T> thats is container with associative find
/// </summary>
template <class T, class U, class = void>
struct is_foundable_helper : std::false_type
{
};
template <class T, class U>
struct is_foundable_helper<T, U, std::void_t<decltype(std::declval<T>().find(std::declval<U>()))>> : std::true_type
{
};
template <class T, class U>
struct is_foundable : is_foundable_helper<typename std::remove_cv<T>::type, typename std::remove_cv<U>::type>
{
};
template <class T, class U>
constexpr bool is_foundable_v = is_foundable<T, U>::value;

/// <summary>
/// Helper structures to determine if template type <T> thats is container with associative find
/// </summary>
template <class T, class = void, class = void, class = void>
struct is_container_helper : std::false_type
{
};
template <class T>
struct is_container_helper<T, std::void_t<decltype(std::declval<T>().begin())>, std::void_t<decltype(std::declval<T>().end())>, std::void_t<decltype(std::declval<T>().size())>> : std::true_type
{
};
template <class T>
struct is_container : is_container_helper<typename std::remove_cv<T>::type>
{
};
template <class T>
constexpr bool is_container_v = is_container<T>::value;

/// <summary>
/// Helper structures to determine if template type <T> thats is concurrent container with associative find
/// </summary>
template <class T, class = void, class = void>
struct is_concurrent_container_helper : std::false_type
{
};
template <class T>
struct is_concurrent_container_helper<T, std::void_t<decltype(std::declval<T>().Concurrent())>, std::void_t<decltype(std::declval<T>().Exclusive())>> : std::true_type
{
};
template <class T>
struct is_concurrent_container : is_concurrent_container_helper<typename std::remove_cv<T>::type>
{
};
template <class T>
constexpr bool is_concurrent_container_v = is_concurrent_container<T>::value;

/// <summary>
/// Helper structures to determine if template type <T> is iterator
/// </summary>
template <class T, class = void>
struct is_iterator_helper : std::false_type
{
};
template <class T>
struct is_iterator_helper<T, typename std::enable_if<!std::is_same<typename std::iterator_traits<T>::value_type, void>::value>::type> : std::true_type
{
};
template <class T>
struct is_iterator : is_iterator_helper<typename std::remove_cv<T>::type>
{
};
template <class T>
constexpr bool is_iterator_v = is_iterator<T>::value;

/// <summary>
/// Helper structures to determine if template type <T> is explicitly convertible
/// </summary>
template <class T, class U>
struct is_explicitly_convertible : std::integral_constant<bool, std::is_constructible_v<U, T> && !std::is_convertible_v<T, U>>
{
};

template <class T, class U>
constexpr bool is_explicitly_convertible_v = is_explicitly_convertible<T, U>::value;

/// <summary>
/// Helper concept to determine if template type <T> is condition variable
/// </summary>
template <class TCondition, class TLock>
concept condition_variable = requires(TCondition& cv, TLock& lock)
{
	{
		cv.notify_one()
	}
	noexcept;
	{
		cv.notify_all()
	}
	noexcept;
	{
		cv.wait(lock)
	};
};

/// <summary>
/// Helper concept to determine if template type <T> is condition variable with pred
/// </summary>
template <class TCondition, class TLock, class TPredicate>
concept condition_variable_pred = condition_variable<TCondition, TLock>&& requires(TCondition& cv, TLock& lock, TPredicate&& pred)
{
	{
		cv.wait(lock, std::move(pred))
	};
};

} //namespace Constraints
