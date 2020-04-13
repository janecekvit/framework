#include "ThreadPool.h"
#include <thread>

ThreadPool::Worker::Worker(ThreadPool& oParentPool)
	: m_oParentPool(oParentPool)
{
	m_oThread = std::thread(&ThreadPool::Worker::_Work, this);
}

ThreadPool::Worker::~Worker()
{
	if (m_oThread.joinable())
		m_oThread.join();
}

void ThreadPool::Worker::_Work()
{
	for (auto&& fnCurrentTask = m_oParentPool.GetTask(); fnCurrentTask; fnCurrentTask = m_oParentPool.GetTask())
	{
		try
		{ //execute task
			(*fnCurrentTask)();
		}
		catch (const std::exception& ex)
		{
			m_oParentPool.ErrorCallback(ex);
		}
	}
	int exit = 0;
}

ThreadPool::ThreadPool(_In_ const size_t uiPoolSize, WorkerErrorCallback&& fnCallback)
	: m_fnErrorCallback(fnCallback)
{
	for (size_t uCount = 0; uCount < uiPoolSize; uCount++)
		m_oWorkers.emplace_back(*this);
}


ThreadPool::~ThreadPool()
{
	//Finish thread pool and wait for 
	m_bEndFlag = true;
	m_cvPoolEvent.notify_all();
	m_oWorkers.clear();
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
	oScope.Wait(WaitEvent(), [&oScope, this]()
	{
		return oScope->empty() || Exit();
	});
}

std::optional<ThreadPool::Task> ThreadPool::GetTask() noexcept
{
	// Wait for the signal and unblock queue until the signal will be received
	auto&& oScope = m_queueTask.Exclusive();
	oScope.Wait(Event(), [&oScope, this]()
	{
		if (oScope->empty())
			m_cvWaitEvent.notify_all();

		return !oScope->empty() || Exit();
	});

	if (Exit())
		return {};

	auto fnCurrentTask = oScope->front();
	oScope->pop();
	return fnCurrentTask;
}

Concurrent::Queue<IThreadPool::Task>& ThreadPool::Queue() noexcept
{
	return m_queueTask;
}

std::condition_variable_any& ThreadPool::Event() const noexcept
{
	return m_cvPoolEvent;
}

std::condition_variable_any& ThreadPool::WaitEvent() const noexcept
{
	return m_cvWaitEvent;
}

bool ThreadPool::Exit() const noexcept
{
	return m_bEndFlag;
}

void ThreadPool::ErrorCallback(const std::exception& ex) noexcept
{
	m_fnErrorCallback(ex);
}
