#include "ThreadPool.h"

#include <thread>

ThreadPool::Worker::Worker(ThreadPool& oParentPool, std::optional<WorkerCallback>&& optTask)
	: m_oParentPool(oParentPool)
	, m_optTask(std::move(optTask))
{
	m_oThread = std::thread(&ThreadPool::Worker::_Work, this);
}

ThreadPool::Worker::~Worker()
{
	if (m_oThread.joinable())
		m_oThread.join();
}

std::thread::id ThreadPool::Worker::Id() const noexcept
{
	return m_oThread.get_id();
}

void ThreadPool::Worker::_Work()
{
	for (auto&& fnCurrentTask = m_oParentPool._GetTask(); fnCurrentTask; fnCurrentTask = m_oParentPool._GetTask())
	{
		try
		{ //execute task
			(*fnCurrentTask)();
		}
		catch (const std::exception& ex)
		{
			m_oParentPool._ErrorCallback(ex);
		}

		//Check worker's callback to check if the processing is necessary to exit.
		if (m_optTask && (*m_optTask)())
			return;
	}
}

ThreadPool::ThreadPool(_In_ size_t uiPoolSize, WorkerErrorCallback&& fnCallback)
	: ThreadPool(uiPoolSize, std::move(fnCallback), std::nullopt)
{
}

ThreadPool::ThreadPool(size_t uiPoolSize, WorkerErrorCallback&& fnCallback, std::optional<WorkerCallback>&& optTask)
	: m_fnErrorCallback(fnCallback)
{
	_AddWorkers(uiPoolSize, std::move(optTask));
}

ThreadPool::~ThreadPool()
{
	//Finish thread pool and wait for
	m_bEndFlag = true;
	m_cvPoolEvent.notify_all();
	m_oWorkers.Exclusive()->clear();
}

void ThreadPool::AddTask(Task&& fn) noexcept
{
	if (m_bEndFlag)
		return;

	//Add new task to queue and inform workers about it
	m_queueTask.Exclusive()->emplace(std::move(fn));
	m_cvPoolEvent.notify_one();
}

void ThreadPool::WaitAll() const noexcept
{
	auto&& oScope = m_queueTask.Concurrent();
	oScope.Wait(_WaitEvent(), [&oScope, this]()
		{
			return oScope->empty() || _Exit();
		});
}

size_t ThreadPool::Size() const noexcept
{
	return m_queueTask.Concurrent()->size();
}

size_t ThreadPool::PoolSize() const noexcept
{
	return m_oWorkers.Concurrent()->size();
}

std::optional<ThreadPool::Task> ThreadPool::_GetTask() noexcept
{
	// Wait for the signal and unblock queue until the signal will be received
	auto&& oScope = m_queueTask.Exclusive();
	oScope.Wait(_Event(), [&oScope, this]()
		{
			if (oScope->empty())
				m_cvWaitEvent.notify_all();

			return !oScope->empty() || _Exit();
		});

	if (_Exit())
		return {};

	auto fnCurrentTask = oScope->front();
	oScope->pop();
	return fnCurrentTask;
}

void ThreadPool::_AddWorkers(_In_ size_t uWorkerCount, std::optional<WorkerCallback>&& optTask) noexcept
{
	for (size_t uCount = 0; uCount < uWorkerCount; uCount++)
		m_oWorkers.Exclusive()->emplace_back(*this, std::optional<WorkerCallback>(optTask));
}

void ThreadPool::_ErrorCallback(const std::exception& ex) noexcept
{
	m_fnErrorCallback(ex);
}

Concurrent::Queue<IThreadPool::Task>& ThreadPool::_Queue() noexcept
{
	return m_queueTask;
}

Concurrent::List<ThreadPool::Worker>& ThreadPool::_Pool() noexcept
{
	return m_oWorkers;
}

std::condition_variable_any& ThreadPool::_Event() const noexcept
{
	return m_cvPoolEvent;
}

std::condition_variable_any& ThreadPool::_WaitEvent() const noexcept
{
	return m_cvWaitEvent;
}

bool ThreadPool::_Exit() const noexcept
{
	return m_bEndFlag;
}

void ThreadPool::_SetExit() noexcept
{
	m_bEndFlag = true;
}
