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
	for (;;)
	{
		// Wait for the signal and unblock queue until the signal will be received
		auto&& oScope = m_oParentPool.m_queueTask.Exclusive();
		oScope.Wait(m_oParentPool.Event());
		oScope.Wait(m_oParentPool.Event(), [&oScope, this]()
		{
			return !oScope->empty() || m_oParentPool.Exit();
		});

		if (m_oParentPool.Exit())
			return;

		// retrieve current task
		auto fnCurrentTask = oScope->front();
		oScope->pop();
		oScope.Release();

		try
		{ //execute the input task
			fnCurrentTask();
		}
		catch (const std::exception& ex)
		{
			m_oParentPool.ErrorCallback(ex);
		}
	}
}

ThreadPool::ThreadPool(_In_ const size_t uiPoolSize, WorkerErrorCallback&& fnCallback)
	: m_fnErrorCallback(fnCallback)
{
	for (size_t uCount = 0; uCount < uiPoolSize; uCount++)
		m_ListOfWorkers.emplace_back(*this);

	m_cvPoolEvent2.NotifyOne(1);
	m_cvPoolEvent2.NotifyAll(0);	
}


ThreadPool::~ThreadPool()
{
	//Set End Flag and Notify Finish
	m_bEndFlag = true;
	m_cvPoolEvent.notify_all();
}

void ThreadPool::AddTask(Task&& fn) noexcept
{
	if (m_bEndFlag)
		return;

	//Add new task to queue and inform workers about it
	m_queueTask.Exclusive()->emplace(std::move(fn));
	m_cvPoolEvent.notify_one();
}

Concurrent::Queue<IThreadPool::Task>& ThreadPool::Queue() noexcept
{
	return m_queueTask;
}

std::condition_variable_any& ThreadPool::Event() noexcept
{
	return m_cvPoolEvent;
}

bool ThreadPool::Exit() const noexcept
{
	return m_bEndFlag;
}

void ThreadPool::ErrorCallback(const std::exception& ex) noexcept
{
	m_fnErrorCallback(ex);
}
