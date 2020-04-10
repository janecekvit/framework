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
	std::function<void()> fnCurrentTask;
	for (;;)
	{
		//Create Unique Lock which owns Mutex object and lock it until wait or block end
		std::unique_lock<std::mutex> mtxQueueLock(m_oParentPool.m_mxtQueueLock);

		//Check If queue is empty thread wait for next task
		if (m_oParentPool.m_queueTask.empty() && !m_oParentPool.m_bEndFlag)
		{
			m_oParentPool.m_cvPoolEvent.wait(mtxQueueLock);
			continue;
		}

		//Check if Queue is empty and End flag is set
		if (m_oParentPool.m_queueTask.empty() && m_oParentPool.m_bEndFlag)
			return;

		fnCurrentTask = m_oParentPool.m_queueTask.front();
		m_oParentPool.m_queueTask.pop();
		mtxQueueLock.unlock();

		//Execute Task if is added
		fnCurrentTask();
	}
}

ThreadPool::ThreadPool(_In_ const size_t uiPoolSize)
{
	for (size_t uCount = 0; uCount < uiPoolSize; uCount++)
		m_ListOfWorkers.emplace_back(*this);
}


ThreadPool::~ThreadPool()
{
	//Set End Flag and Notify Finish
	m_bEndFlag = true;
	m_cvPoolEvent.notify_all();
}

void ThreadPool::AddTask(_In_ const std::function<void()> &fn)
{
	if (m_bEndFlag)
		return;

	//Add Task to locked queue 
	std::unique_lock<std::mutex> mtxThreadLock(m_mxtQueueLock);
	m_queueTask.emplace(fn);
	mtxThreadLock.unlock();

	//Notify worker Thread
	m_cvPoolEvent.notify_one();
}
