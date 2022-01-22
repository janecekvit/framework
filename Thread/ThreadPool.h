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
#include "IThreadPool.h"
#include "extensions/concurrent.h"

#include <atomic>
#include <list>
#include <queue>

/// <summary>
/// Fixed sized thread pool that executes the task from the queue.
/// </summary>
class ThreadPool
	: public virtual IThreadPool
{
public:
	using WorkerCallback	  = typename std::function<bool()>;
	using WorkerErrorCallback = typename std::function<void(const std::exception&)>;

protected:
	class Worker
	{
	public:
		Worker(ThreadPool& oParentPool, std::optional<WorkerCallback>&& optTask);

		virtual ~Worker();

		std::thread::id Id() const noexcept;

	protected:
		void _Work();

	private:
		ThreadPool& m_oParentPool; //dependency
		std::thread m_oThread;
		std::optional<WorkerCallback> m_optTask;
	};

public:
	ThreadPool(size_t uiPoolSize, WorkerErrorCallback&& fnCallback);

protected:
	ThreadPool(size_t uiPoolSize, WorkerErrorCallback&& fnCallback, std::optional<WorkerCallback>&& optTask);

public:
	virtual ~ThreadPool();

public: // IThreadPool interface
	void AddTask(Task&& fn) noexcept override;
	void WaitAll() const noexcept override;
	size_t Size() const noexcept override;
	size_t PoolSize() const noexcept override;

protected:
	std::optional<Task> _GetTask() noexcept;
	void _AddWorkers(size_t uWorkerCount, std::optional<WorkerCallback>&& optTask) noexcept;
	void _ErrorCallback(const std::exception& ex) noexcept;

protected: //getters && setters
	janecekvit::concurrent::queue<Task>& _Queue() noexcept;
	janecekvit::concurrent::list<Worker>& _Pool() noexcept;
	std::condition_variable_any& _Event() const noexcept;
	std::condition_variable_any& _WaitEvent() const noexcept;
	bool _Exit() const noexcept;
	void _SetExit() noexcept;

private:
	std::atomic<bool> m_bEndFlag = false;
	const WorkerErrorCallback m_fnErrorCallback;

	mutable std::condition_variable_any m_cvPoolEvent;
	mutable std::condition_variable_any m_cvWaitEvent;

	janecekvit::concurrent::queue<Task> m_queueTask;
	janecekvit::concurrent::list<Worker> m_oWorkers;
};