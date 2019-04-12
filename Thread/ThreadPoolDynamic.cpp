#include "ThreadPoolDynamic.h"

CThreadPoolDynamic::CThreadPoolDynamic(_In_ const size_t uiPoolSize, _In_ const double uDifference)
	: m_dDifference(uDifference)
{
	//Minimum pool size is 4
	if (uiPoolSize > m_uMinimumPoolSize)
		m_uMinimumPoolSize = uiPoolSize;

	_AddWorkers(m_uMinimumPoolSize);
	m_pCurrentThread = new std::thread(&CThreadPoolDynamic::_PoolController, this);
}


CThreadPoolDynamic::~CThreadPoolDynamic()
{
	//Set End Flag and Notify Finish
	m_uConditionEvents = EConditionEvents::eExit;
	m_cvPoolEvent.notify_one();

	//Wait for End
	std::mutex mtxEndPool;
	std::unique_lock<std::mutex> unqEndLock(mtxEndPool);

	m_cvPoolEndEvent.wait(unqEndLock);
}

void CThreadPoolDynamic::AddTask(_In_ const std::function<void()> &fn)
{
	if (m_uConditionEvents == EConditionEvents::eExit)
		return;
	
	size_t uCurrentQueueSize = 0;

	//lock queue and add task
	std::unique_lock<std::mutex> unqQueueLock(m_mxtQueueLock);
	m_queueTask.emplace(fn);
	uCurrentQueueSize = m_queueTask.size();
	unqQueueLock.unlock();

	//Notify Worker Thread
	m_cvQueueEvent.notify_one();

	//Check if we need to deallocate new number of threads
	std::unique_lock<std::mutex> unqPoolLock(m_mxtPoolLock);

	//Check if Task Queue is bigger that multiplier of list of workers;
	if (uCurrentQueueSize > m_dDifference * m_ListOfWorkers.size())
	{
		m_uConditionEvents = EConditionEvents::eMagnify;
		m_cvPoolEvent.notify_one();
	}

	//Check if Task Queue is bigger that minimum pool size and smaller that multiplier of list of workers
	if ((m_ListOfWorkers.size() > m_uMinimumPoolSize) && ((uCurrentQueueSize * m_uPoolDeallocationMultiplier) < m_ListOfWorkers.size()))
	{
		m_uConditionEvents = EConditionEvents::eReduce;
		m_cvPoolEvent.notify_one();
	}
}

size_t CThreadPoolDynamic::GetPoolSize()
{
	//Lock Pool by Unique lock
	std::unique_lock<std::mutex> unqPoolLock(m_mxtPoolLock);
	return m_ListOfWorkers.size();
}


void CThreadPoolDynamic::_PoolController()
{
	for (;;)
	{
		//Wait for signal
		std::unique_lock<std::mutex> unqPoolLock(m_mxtPoolLock);
		m_cvPoolEvent.wait(unqPoolLock);

		if (m_uConditionEvents == EConditionEvents::eExit)
		{
			//Send signals to Workers
			m_uDeallocationPoolSize = m_ListOfWorkers.size();
			m_cvQueueEvent.notify_all();

			//Wait for Workers finish
			for (auto &oWorker : m_ListOfWorkers)
				oWorker.Join();

			//pool thread going to exit
			m_cvPoolEndEvent.notify_one();
			return;
		}
		else if (m_uConditionEvents == EConditionEvents::eMagnify)
		{
			if (!IsDeallocationEnabled())
				_AddWorkers(m_ListOfWorkers.size());
		}
		else if (m_uConditionEvents == EConditionEvents::eReduce)
		{
			if (!IsDeallocationEnabled())
				m_uDeallocationPoolSize = m_ListOfWorkers.size() / m_uPoolMultiplier;
		}

		else if (m_uConditionEvents == EConditionEvents::eDealloc)
		{
			//release workers
			for (std::list<CWorkerDynamic>::iterator it = m_ListOfWorkers.begin(); it != m_ListOfWorkers.end();)
			{
				//If Worker Ends
				if (it->IsWorkerEnd())
				{
					//Join workrer and release resources
					it->Join();
					m_ListOfWorkers.erase(it++);
				}
				else //Or Get next
					++it;
			}
		}

		//Set default state
		m_uConditionEvents = EConditionEvents::eNone;
	}
}

void CThreadPoolDynamic::_AddWorkers(_In_ const size_t uWorkerCount)
{
	for (size_t uCount = 0; uCount < uWorkerCount; uCount++)
		m_ListOfWorkers.emplace_back(this, std::bind(&CThreadPoolDynamic::_PoolCallback, this));
}

void CThreadPoolDynamic::_PoolCallback()
{
	if (m_uConditionEvents == EConditionEvents::eExit)
		return;
	
	//Decrement reference counter
	m_uDeallocationPoolSize--;

	//If zero references
	if (!IsDeallocationEnabled())
	{
		//Notify controller to delete memory (Garbage Collector)
		m_uConditionEvents = EConditionEvents::eDealloc;
		m_cvPoolEvent.notify_one();
	}
}