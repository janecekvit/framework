/*
Copyright (c) 2020 Vit janecek <mailto:janecekvit@outlook.com>.
All rights reserved.

constraints.h
Purpose:	header file contains set of extended constraints to describe stl containers

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.16 15/10/2020
*/

#pragma once
#include <algorithm>
#include <any>
#include <concepts>
#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <semaphore>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>

/// Namespace owns set of extended constraints to describe stl containers
namespace janecekvit::constraints
{

class default_exception_callback
{
};

/// <summary>
/// Helper structures to determine if template type <_T> is std::shared_ptr
/// </summary>
template <class _T>
struct is_shared_ptr_helper : std::false_type
{
};
template <class _T>
struct is_shared_ptr_helper<std::shared_ptr<_T>> : std::true_type
{
};
template <class _T>
struct is_shared_ptr : is_shared_ptr_helper<typename std::remove_cv<_T>::type>
{
};
template <class _T>
constexpr bool is_shared_ptr_v = is_shared_ptr<_T>::value;

/// <summary>
/// Helper structures to determine if template type <_T> is std::unique_ptr
/// </summary>
template <class _T>
struct is_unique_ptr_helper : std::false_type
{
};
template <class _T>
struct is_unique_ptr_helper<std::unique_ptr<_T>> : std::true_type
{
};
template <class _T>
struct is_unique_ptr : is_unique_ptr_helper<typename std::remove_cv<_T>::type>
{
};
template <class _T>
constexpr bool is_unique_ptr_v = is_unique_ptr<_T>::value;

/// <summary>
/// Helper structures to determine if template type <_T> is std::pair
/// </summary>
template <class... _Args>
struct is_pair_helper : std::false_type
{
};
template <class... _Args>
struct is_pair_helper<std::pair<_Args...>> : std::true_type
{
};
template <class... _Args>
struct is_pair : is_pair_helper<typename std::remove_cv<_Args...>::type>
{
};
template <class _T>
constexpr bool is_pair_v = is_pair<_T>::value;

/// <summary>
/// Helper structures to determine if template type <_T> thats is container with associative find
/// </summary>
template <class _T, class _U, class = void>
struct is_foundable_helper : std::false_type
{
};
template <class _T, class _U>
struct is_foundable_helper<_T, _U, std::void_t<decltype(std::declval<_T>().find(std::declval<_U>()))>> : std::true_type
{
};
template <class _T, class _U>
struct is_foundable : is_foundable_helper<typename std::remove_cv<_T>::type, typename std::remove_cv<_U>::type>
{
};
template <class _T, class _U>
constexpr bool is_foundable_v = is_foundable<_T, _U>::value;

/// <summary>
/// Helper structures to determine if template type <_T> thats is container with associative find
/// </summary>
template <class _T, class = void, class = void, class = void>
struct is_container_helper : std::false_type
{
};
template <class _T>
struct is_container_helper<_T, std::void_t<decltype(std::declval<_T>().begin())>, std::void_t<decltype(std::declval<_T>().end())>, std::void_t<decltype(std::declval<_T>().size())>> : std::true_type
{
};
template <class _T>
struct is_container : is_container_helper<typename std::remove_cv<_T>::type>
{
};
template <class _T>
constexpr bool is_container_v = is_container<_T>::value;

/// <summary>
/// Helper structures to determine if template type <_T> thats is concurrent container with associative find
/// </summary>
template <class _T, class = void, class = void>
struct is_concurrent_container_helper : std::false_type
{
};
template <class _T>
struct is_concurrent_container_helper<_T, std::void_t<decltype(std::declval<_T>().concurrent())>, std::void_t<decltype(std::declval<_T>().exclusive())>> : std::true_type
{
};
template <class _T>
struct is_concurrent_container : is_concurrent_container_helper<typename std::remove_cv<_T>::type>
{
};
template <class _T>
constexpr bool is_concurrent_container_v = is_concurrent_container<_T>::value;

/// <summary>
/// Helper structures to determine if template type <_T> is iterator
/// </summary>
template <class _T, class = void>
struct is_iterator_helper : std::false_type
{
};
template <class _T>
struct is_iterator_helper<_T, typename std::enable_if<!std::is_same<typename std::iterator_traits<_T>::value_type, void>::value>::type> : std::true_type
{
};
template <class _T>
struct is_iterator : is_iterator_helper<typename std::remove_cv<_T>::type>
{
};
template <class _T>
constexpr bool is_iterator_v = is_iterator<_T>::value;

/// <summary>
/// Helper structures to determine if template type <_T> is tuple
/// </summary>
template <typename T>
struct is_tuple : std::false_type
{
};

template <typename... Args>
struct is_tuple<std::tuple<Args...>> : std::true_type
{
};

template <typename T>
constexpr bool is_tuple_v = is_tuple<T>::value;

/// <summary>
/// Helper structures to determine if template type <_T> is initializer_list
/// </summary>
template <typename T>
struct is_initializer_list : std::false_type
{
};

template <typename T>
struct is_initializer_list<std::initializer_list<T>> : std::true_type
{
};

template <typename T>
constexpr bool is_initializer_list_v = is_initializer_list<T>::value;

/// <summary>
/// Helper structures to determine if template type <_T> is explicitly convertible
/// </summary>
template <class _T, class _U>
struct is_explicitly_convertible : std::integral_constant<bool, std::is_constructible_v<_U, _T> && !std::is_convertible_v<std::decay_t<_T>, _U>>
{
};

template <class _T, class _U>
constexpr bool is_explicitly_convertible_v = is_explicitly_convertible<_T, _U>::value;

template <class _T, class _U>
constexpr bool is_convertible_v = std::is_convertible_v<std::decay_t<_T>, _U> && !std::is_same_v<std::decay_t<_T>, _U>;

#ifdef __cpp_lib_concepts

template <class _Condition, class _Lock>
concept condition_variable_notify = requires(_Condition& cv, _Lock& lock) {
	{
		cv.notify_one()
	} noexcept;
	{
		cv.notify_all()
	} noexcept;
	{
		cv.wait(lock)
	};
};

template <class _Condition, class _Lock, class _Predicate>
concept condition_variable_pred = condition_variable_notify<_Condition, _Lock> && requires(_Condition& cv, _Lock& lock, _Predicate&& pred) {
	{
		cv.wait(lock, std::move(pred))
	};
};

template <class _Fmt>
concept format_string_view = std::is_constructible_v<std::string_view, _Fmt>;

template <class _Fmt>
concept format_wstring_view = std::is_constructible_v<std::wstring_view, _Fmt>;

template <class _Fmt>
concept format_view = format_string_view<_Fmt> || format_wstring_view<_Fmt>;

template <class _String>
concept format_outout = std::is_same_v<_String, std::string> || std::is_same_v<_String, std::wstring>;

template <class _Enum>
concept enum_type = std::is_enum_v<_Enum>;

template <class _String>
concept string_type = std::is_same_v<_String, std::string> || std::is_same_v<_String, std::wstring> || std::is_same_v<_String, std::u8string> || std::is_same_v<_String, std::u16string> || std::is_same_v<_String, std::u32string>;

template <class _Condition>
concept condition_variable_type = std::is_same_v<std::condition_variable, _Condition> || std::is_same_v<std::condition_variable_any, _Condition>;

#ifdef __cpp_lib_semaphore
template <class _Semaphore, ptrdiff_t _Least_max_value = std::_Semaphore_max>
concept semaphore_type = std::is_same_v<std::binary_semaphore, _Semaphore> || std::is_same_v<std::counting_semaphore<_Least_max_value>, _Semaphore>;
#endif // __cpp_lib_semaphore

#ifdef __cpp_lib_semaphore
template <class _Semaphore>
concept binary_semaphore_type = std::is_same_v<std::binary_semaphore, _Semaphore>;
#endif // __cpp_lib_semaphore

#endif

} // namespace janecekvit::constraints
