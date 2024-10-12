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

CThreadPool.h
Purpose: header file of static thread pool class

@author: Vit Janecek
@mailto: janecekvit@gmail.com
@version 1.05 01/03/2018
*/

#pragma once
#include "synchronization/concurrent.h"
// #include "thread/thread_pool.h"
#include "synchronization/wait_for_multiple_signals.h"

#include <atomic>
#include <future>
#include <list>
#include <queue>

namespace janecekvit::thread
{

/// <summary>
/// Fixed sized thread pool that executes the task from the queue.
/// </summary>
class sync_thread_pool
//: public virtual thread_pool
{
public:
	using _Task			 = typename std::function<void()>;
	using _ErrorCallback = typename std::function<void(const std::exception&)>;

private:
	class worker
	{
		template <class... Args>
		worker(sync_thread_pool& parent);
		~worker();

		std::thread _thread;
	};

public:
	sync_thread_pool(size_t uiPoolSize);
	sync_thread_pool(size_t uiPoolSize, _ErrorCallback&& callback);

public:
	virtual ~sync_thread_pool();

public: // IThreadPool interface
	template <class... _Args>
		requires std::is_invocable_v<std::packaged_task<void(_Args...)>, _Args...>
	void add_task(std::packaged_task<void(_Args...)>&& fn) noexcept
	{
		_tasks.exclusive()->emplace([x = std::move(fn)]
			{
				x();
			});
	}

	template <class _R, class... _Args>
		requires std::is_invocable_r_v<_R, std::packaged_task<_R(_Args...)>, _Args...>
	std::future<_R> add_waitable_task(std::packaged_task<_R(_Args...)>&& fn) noexcept
	{
		auto future = fn.get_future();
		_tasks.exclusive()->emplace([x = std::move(fn)]
			{
				x();
			});

		return future;
	}
	size_t size() const noexcept;
	size_t pool_size() const noexcept;

protected: // getters && setters
	void _work();
	std::list<std::thread> _add_workers(size_t uWorkerCount) noexcept;
	std::optional<_Task> _get_task() noexcept;

private:
	enum class state
	{
		Ready,
		Exit
	};

	synchronization::wait_for_multiple_signals<state> _event;
	synchronization::concurrent::queue<_Task> _tasks;
	const std::list<std::thread> _workers;
	const std::optional<_ErrorCallback> _callback;
};

} // namespace janecekvit::thread