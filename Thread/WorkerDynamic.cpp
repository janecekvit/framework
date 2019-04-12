#include "WorkerDynamic.h"

CWorkerDynamic::CWorkerDynamic(_Inout_ CThreadPoolDynamic *pParentPool, _In_ const std::function<void()> &pCallback)
	: m_pParentPool(pParentPool)
	, m_pCallback(pCallback)
{
	m_pCurrentThread = std::unique_ptr<std::thread>(new std::thread(&CWorkerDynamic::_Work, this));
}

CWorkerDynamic::~CWorkerDynamic()
{
}


void CWorkerDynamic::Join()
{
	if (m_pCurrentThread->joinable())
		m_pCurrentThread->join();
}


void CWorkerDynamic::_Work()
{
	std::function<void()> fnCurrentTask;

	for (;;)
	{
		//Create Unique Lock which owns Mutex object and lock it until wait or block end
		std::unique_lock<std::mutex> unqQueueLock(m_pParentPool->m_mxtQueueLock);

		//Check if deallocation is enabled
		if (m_pParentPool->IsDeallocationEnabled())
		{
			//Callback and decrement reference counter
			m_pCallback();
			m_bWorkerEnd = true;
			return;
		}

		//Check If queue is empty, thread will wait for next task
		if (!m_pParentPool->m_queueTask.empty())
		{
			//Get Task from Task Queue
			fnCurrentTask = m_pParentPool->m_queueTask.front();
			m_pParentPool->m_queueTask.pop();

			//Unlock Queue lock
			unqQueueLock.unlock();

			//Execute Task if is added
			try
			{
				fnCurrentTask();
			}
			catch (...) {}
		}
		else
			m_pParentPool->m_cvQueueEvent.wait(unqQueueLock);

	}
}
