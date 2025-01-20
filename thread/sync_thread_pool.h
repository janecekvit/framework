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

sync_thread_pool.h
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

namespace janecekvit::thread
{

/// <summary>
/// Fixed sized thread pool that executes the task from the queue.
/// </summary>
class sync_thread_pool
{
public:
#ifdef __cpp_lib_move_only_function
	using task = typename std::move_only_function<void()>;
#else

	class move_only_function
	{
		struct callable_base
		{
			virtual ~callable_base() = default;
			virtual void operator()() = 0;
		};

		template <typename _Fn>
		struct callable : callable_base
		{
			callable(_Fn&& f)
				: _function(std::move(f))
			{
			}

			void operator()() override
			{
				_function();
			}

		private:
			_Fn _function;
		};

	public:
		move_only_function() = default;
		virtual ~move_only_function() = default;

		template <typename _F>
		move_only_function(_F&& f)
			: _function(std::make_unique<callable<_F>>(std::move(f)))
		{
		}

		move_only_function(move_only_function&& other) noexcept
			: _function(std::move(other._function))
		{
		}

		move_only_function& operator=(move_only_function&& other) noexcept
		{
			if (this != &other)
				_function = std::move(other._function);
			return *this;
		}

		move_only_function(const move_only_function&) = delete;
		move_only_function& operator=(const move_only_function&) = delete;

		void operator()()
		{
			if (_function)
				_function->operator()();
		}

		explicit operator bool() const noexcept
		{
			return _function != nullptr;
		}

	private:
		std::unique_ptr<callable_base> _function;
	};

	using task = typename move_only_function;
#endif // __cpp_lib_move_only_function

private:
	class worker
	{
	public:
		worker(sync_thread_pool& parent);
		virtual ~worker();

	private:
#ifdef __cpp_lib_jthread
		std::jthread _thread;
#else
		std::thread _thread;
#endif
	};

public:
	sync_thread_pool(size_t size);

public:
	virtual ~sync_thread_pool();

	template <typename _Fn>
		requires std::is_invocable_v<_Fn>
	void add_task(_Fn&& fn) noexcept
	{
		add_task(std::packaged_task<void()>(std::forward<_Fn>(fn)));
	}

	template <class... _Args>
		requires std::is_invocable_v<std::packaged_task<void()>, _Args...>
	void add_task(std::packaged_task<void()>&& fn) noexcept
	{
		_tasks.exclusive()->emplace([x = std::move(fn)]() mutable
			{
				x();
			});
		_event.signalize(state::Ready);
	}

	template <typename _Fn>
		requires std::is_invocable_v<_Fn>
	[[nodiscard]] auto add_waitable_task(_Fn&& fn) noexcept
	{
		return add_waitable_task(std::packaged_task<std::invoke_result_t<_Fn>()>(std::forward<_Fn>(fn)));
	}

	template <class _R, class... _Args>
		requires std::is_invocable_v<std::packaged_task<_R()>, _Args...>
	[[nodiscard]] std::future<_R> add_waitable_task(std::packaged_task<_R()>&& fn) noexcept
	{
		auto future = fn.get_future();
		_tasks.exclusive()->emplace([x = std::move(fn)]() mutable
			{
				x();
			});

		_event.signalize(state::Ready);
		return future;
	}

	size_t size() const noexcept;
	size_t pool_size() const noexcept;

protected: // getters && setters
	void _work();
	std::list<worker> _add_workers(size_t count) noexcept;
	std::optional<task> _get_task() noexcept;

private:
	enum class state
	{
		Ready,
		Exit
	};

	std::atomic<size_t> _exited_workers_count = 0; // TODO: remove When condition variable notify all finished
	synchronization::wait_for_multiple_signals<state> _event;
	synchronization::concurrent::queue<task> _tasks;
	const std::list<worker> _workers;
};

} // namespace janecekvit::thread
