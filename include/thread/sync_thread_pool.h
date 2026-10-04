/*
Copyright (c) 2018 Vit Janecek. All rights reserved.

sync_thread_pool.h
Purpose:	header file of static thread pool class

@author:  Vit Janecek
@mailto:  <mailto:janecekvit@outlook.com>
@version: 1.05 15/10/2024
*/

#pragma once

#include "compatibility/compiler_support.h"

#include <condition_variable>
#include <functional>
#include <future>
#include <list>
#include <mutex>
#include <queue>
#include <ranges>
#include <thread>

namespace janecekvit::thread
{

#if defined(HAS_JTHREAD)

/// <summary>
/// Fixed sized thread pool that executes the task from the queue.
/// </summary>
class sync_thread_pool
{
public:
#ifdef __cpp_lib_move_only_function
	using task = std::move_only_function<void()>;
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

	using task = move_only_function;
#endif // __cpp_lib_move_only_function

public:
	sync_thread_pool(size_t size)
		: _workers(_add_workers(size))
	{
	}

public:
	virtual ~sync_thread_pool()
	{
		{
			std::scoped_lock lck(_lock);
			for (auto& t : const_cast<std::list<std::jthread>&>(_workers))
			{
				t.request_stop();
			}
		}

		_event.notify_all();
	}

	template <typename _Fn>
		requires std::is_invocable_v<_Fn>
	void add_task(_Fn&& fn) noexcept
	{
		std::scoped_lock lck(_lock);
		_tasks.emplace([x = std::forward<_Fn>(fn)]() mutable
			{
				try
				{
					std::invoke(x);
				}
				catch (...)
				{
				}
			});
		_event.notify_one();
	}

	void add_task(std::packaged_task<void()>&& fn) noexcept
	{
		std::scoped_lock lck(_lock);
		_tasks.emplace([x = std::move(fn)]() mutable
			{
				x();
			});
		_event.notify_one();
	}

	template <typename _Fn>
		requires std::is_invocable_v<_Fn>
	[[nodiscard]] auto add_waitable_task(_Fn&& fn) noexcept
	{
		return add_waitable_task(std::packaged_task<std::invoke_result_t<_Fn>()>(std::forward<_Fn>(fn)));
	}

	template <class _R>
	[[nodiscard]] std::future<_R> add_waitable_task(std::packaged_task<_R()>&& fn) noexcept
	{
		std::scoped_lock lck(_lock);
		auto future = fn.get_future();
		_tasks.emplace([x = std::move(fn)]() mutable
			{
				x();
			});

		_event.notify_one();
		return future;
	}

	size_t size() const noexcept
	{
		std::scoped_lock lck(_lock);
		return _tasks.size();
	}

	size_t pool_size() const noexcept
	{
		return _workers.size();
	}

protected:
	void _work(std::stop_token token)
	{
		for (;;)
		{
			std::unique_lock lck(_lock);
			_event.wait(lck, [this, &token]()
				{
					return !_tasks.empty() || token.stop_requested();
				});

			if (token.stop_requested() && _tasks.empty())
				return;

			auto fnCurrentTask = std::move(_tasks.front());
			_tasks.pop();
			lck.unlock();

			fnCurrentTask();
		}
	}

	std::list<std::jthread> _add_workers(size_t count) noexcept
	{
		std::list<std::jthread> workers;
		for (size_t counter = 0; counter < count; counter++)
			workers.emplace_back(std::jthread([this](std::stop_token token)
				{
					this->_work(token);
				}));

		return workers;
	}

private:
	mutable std::mutex _lock;
	std::condition_variable _event;
	std::queue<task> _tasks;
	const std::list<std::jthread> _workers;
};

#endif

} // namespace janecekvit::thread
