#pragma once
#include <functional>
#include <future>
#include <optional>

namespace janecekvit::thread
{

///// <summary>
///// Interface implementing methods to create the thread pool that executes tasks from the queue of tasks.
///// </summary>
// class thread_pool
//{
// public:
//	using Task = typename std::function<void()>;
//
// public:
//	thread_pool()		   = default;
//	virtual ~thread_pool() = default;
//
//	/// <summary>
//	/// The method adds a new task to the thread pool to be executed as soon as possible.
//	/// </summary>
//	/// <param name="fn">the input task</param>
//	virtual void add_task(Task&& fn) noexcept = 0;
//
//	virtual std::future<T> add_task(int&& fn) noexcept = 0;
//
//	/// <summary>
//	/// Method synchronously waits until all input tasks in the thread pool will be finished.
//	/// </summary>
//	virtual void wait_for_all() const noexcept = 0;
//
//	/// <summary>
//	/// Get the queue size.
//	/// </summary>
//	virtual size_t size() const noexcept = 0;
//
//	/// <summary>
//	/// Get the thread pool size.
//	/// </summary>
//	virtual size_t pool_size() const noexcept = 0;
// };

} // namespace janecekvit::thread