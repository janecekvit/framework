/*
Copyright (c) 2018 Vit Janecek. All rights reserved.

async.h
Purpose:	header file of static thread pool class

@author:  Vit Janecek
@mailto:  <mailto:janecekvit@outlook.com>
@version: 1.05 15/10/2024
*/

#pragma once
#include "synchronization/concurrent.h"
#include "synchronization/wait_for_multiple_signals.h"

#include <atomic>
#include <functional>
#include <future>
#include <list>
#include <queue>
#include <type_traits>

namespace janecekvit::thread::async
{
/// <summary>
/// Creates a new asynchronous task that invokes the specified function with the given arguments.
/// </summary>
/// <typeparam name="_Fn">The type of the function to be invoked.</typeparam>
/// <typeparam name="...Args">The types of the arguments to be passed to the function.</typeparam>
/// <param name="fn">The function to be invoked.</param>
/// <param name="...args">The arguments to be passed to the function.</param>
/// <returns>A std::future object representing the result of the function invocation.</returns>
template <typename _Fn, typename... Args>
	requires std::is_invocable_r_v<std::invoke_result_t<_Fn, Args...>, _Fn, Args...>
[[nodiscard]] std::future<std::invoke_result_t<_Fn, Args...>> create(_Fn&& fn, Args&&... args) noexcept
{
	return std::async(std::launch::async, std::forward<_Fn>(fn), std::forward<Args>(args)...);
}

} // namespace janecekvit::thread::async
