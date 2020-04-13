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
#include <list>
#include <queue>
#include <atomic>

#include "IThreadPool.h"
#include "Framework/Extensions/Concurrent.h"


class ThreadPool 
	: public virtual IThreadPool
{
public:
	using WorkerErrorCallback = typename std::function<void(const std::exception&)>;

private:
	class Worker
	{
	public:
		Worker(ThreadPool& oParentPool);
		virtual ~Worker();

	protected:
		void _Work();

	private:
		ThreadPool& m_oParentPool; //dependency
		std::thread m_oThread;
	};

public:
	ThreadPool(const size_t uiPoolSize, WorkerErrorCallback&& fnCallback);
	virtual ~ThreadPool();

public: // IThreadPool interface
	void AddTask(Task&& fn) noexcept override;
	void WaitAll() const noexcept override;

protected:
	std::optional<Task> GetTask() noexcept;

protected:
	Concurrent::Queue<Task>& Queue() noexcept;
	std::condition_variable_any& Event() const noexcept;
	std::condition_variable_any& WaitEvent() const noexcept;
	bool Exit() const noexcept;
	void ErrorCallback(const std::exception& ex) noexcept;

private:
	Concurrent::Queue<Task> m_queueTask;
	const WorkerErrorCallback m_fnErrorCallback;

	mutable std::condition_variable_any m_cvPoolEvent;
	mutable std::condition_variable_any m_cvWaitEvent;
	std::atomic<bool> m_bEndFlag = false;

	std::list<Worker> m_oWorkers;
};