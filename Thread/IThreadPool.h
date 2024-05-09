#pragma once
#include <functional>
#include <optional>

namespace janecekvit::thread
{

/// <summary>
/// Interface implementing methods to create the thread pool that executes tasks from the queue of tasks.
/// </summary>
class IThreadPool
{
public:
	using Task = typename std::function<void()>;

public:
	IThreadPool()		   = default;
	virtual ~IThreadPool() = default;

	/// <summary>
	/// The method adds a new task to the thread pool to be executed as soon as possible.
	/// </summary>
	/// <param name="fn">the input task</param>
	virtual void AddTask(Task&& fn) noexcept = 0;

	/// <summary>
	/// Method synchronously waits until all input tasks in the thread pool will be finished.
	/// </summary>
	virtual void WaitAll() const noexcept = 0;

	/// <summary>
	/// Get the queue size.
	/// </summary>
	virtual size_t Size() const noexcept = 0;

	/// <summary>
	/// Get the thread pool size.
	/// </summary>
	virtual size_t PoolSize() const noexcept = 0;
};

} // namespace janecekvit::thread