#pragma once
#include <optional>
#include <functional>

class IThreadPool
{
public:
	using Task = typename std::function<void()>;
public:
	IThreadPool() = default;
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
};