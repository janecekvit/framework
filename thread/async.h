/*
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
Copyright (c) 2018 Vit janecek <mailto:janecekvit@gmail.com>.

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

Async.h
Purpose: header file of static thread pool class

@author: Vit Janecek
@mailto: janecekvit@gmail.com
@version 1.05 15/10/2024
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
	return std::async(std::launch::async, std::forward<_Fn>(fn), std::forward<Args...>(args)...);
}

} // namespace janecekvit::thread::async
