#include "ThreadPool.h"

CThreadPool::CThreadPool(_In_ const size_t uiPoolSize)
{
	for (size_t uCount = 0; uCount < uiPoolSize; uCount++)
		m_ListOfWorkers.emplace_back(this);
}


CThreadPool::~CThreadPool()
{
	//Set End Flag and Notify Finish
	m_bEndFlag = true;
	m_cvPoolEvent.notify_all();

	for (auto &oWorker : m_ListOfWorkers)
		oWorker.Join();
}

void CThreadPool::AddTask(_In_ const std::function<void()> &fn)
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
