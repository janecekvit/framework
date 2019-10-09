#include "Worker.h"

CWorker::CWorker(_Inout_ CThreadPool *pParentPool)
	: m_pParentPool(pParentPool)
{
	m_pCurrentThread = std::unique_ptr<std::thread>(new std::thread(&CWorker::Work, this));
}

CWorker::~CWorker()
{
}

void CWorker::Work()
{
	std::function<void()> fnCurrentTask;
	for (;;)
	{
		//Create Unique Lock which owns Mutex object and lock it until wait or block end
		std::unique_lock<std::mutex> mtxQueueLock(m_pParentPool->m_mxtQueueLock);

		//Check If queue is empty thread wait for next task
		if (m_pParentPool->m_queueTask.empty() && !m_pParentPool->m_bEndFlag)
		{
			m_pParentPool->m_cvPoolEvent.wait(mtxQueueLock);
			continue;
		}

		//Check if Queue is empty and End flag is set
		if (m_pParentPool->m_queueTask.empty() && m_pParentPool->m_bEndFlag)
			return;

		fnCurrentTask = m_pParentPool->m_queueTask.front();
		m_pParentPool->m_queueTask.pop();
		mtxQueueLock.unlock();

		//Execute Task if is added
		fnCurrentTask();
	}
}

void CWorker::Join()
{
	if (m_pCurrentThread->joinable())
		m_pCurrentThread->join();
}
