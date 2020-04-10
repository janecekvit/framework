#include "ThreadPoolDynamic.h"


ThreadPoolDynamic::Worker::Worker(ThreadPoolDynamic& oParentPool, _In_ const std::function<void()>& pCallback)
	: m_oParentPool(oParentPool)
	, m_pCallback(pCallback)
{
	m_oThread = std::thread(&ThreadPoolDynamic::Worker::_Work, this);
}

ThreadPoolDynamic::Worker::~Worker()
{
	if (m_oThread.joinable())
		m_oThread.join();
}


void ThreadPoolDynamic::Worker::_Work()
{
	std::function<void()> fnCurrentTask;

	for (;;)
	{
		//Create Unique Lock which owns Mutex object and lock it until wait or block end
		std::unique_lock<std::mutex> unqQueueLock(m_oParentPool.m_mxtQueueLock);

		//Check if deallocation is enabled
		if (m_oParentPool.IsDeallocationEnabled())
		{
			//Callback and decrement reference counter
			m_pCallback();
			m_bWorkerEnd = true;
			return;
		}

		//Check If queue is empty, thread will wait for next task
		if (!m_oParentPool.m_queueTask.empty())
		{
			//Get Task from Task Queue
			fnCurrentTask = m_oParentPool.m_queueTask.front();
			m_oParentPool.m_queueTask.pop();

			//Unlock Queue lock
			unqQueueLock.unlock();

			//Execute Task if is added
			try
			{
				fnCurrentTask();
			}
			catch (...)
			{
			}
		}
		else
			m_oParentPool.m_cvQueueEvent.wait(unqQueueLock);

	}
}


ThreadPoolDynamic::ThreadPoolDynamic(_In_ const size_t uiPoolSize, _In_ const double uDifference)
	: m_dDifference(uDifference)
{
	//Minimum pool size is 4
	if (uiPoolSize > m_uMinimumPoolSize)
		m_uMinimumPoolSize = uiPoolSize;

	_AddWorkers(m_uMinimumPoolSize);
	m_pCurrentThread = new std::thread(&ThreadPoolDynamic::_PoolController, this);
}


ThreadPoolDynamic::~ThreadPoolDynamic()
{
	//Set End Flag and Notify Finish
	m_uConditionEvents = EConditionEvents::eExit;
	m_cvPoolEvent.notify_one();

	//Wait for End
	std::mutex mtxEndPool;
	std::unique_lock<std::mutex> unqEndLock(mtxEndPool);

	m_cvPoolEndEvent.wait(unqEndLock);
}

void ThreadPoolDynamic::AddTask(_In_ const std::function<void()> &fn)
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

size_t ThreadPoolDynamic::GetPoolSize()
{
	//Lock Pool by Unique lock
	std::unique_lock<std::mutex> unqPoolLock(m_mxtPoolLock);
	return m_ListOfWorkers.size();
}


void ThreadPoolDynamic::_PoolController()
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
			m_ListOfWorkers.clear();

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
			for (std::list<Worker>::iterator it = m_ListOfWorkers.begin(); it != m_ListOfWorkers.end();)
			{
				//If Worker Ends
				if (it->IsWorkerEnd())
				{
					//Join workrer and release resources
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

void ThreadPoolDynamic::_AddWorkers(_In_ const size_t uWorkerCount)
{
	for (size_t uCount = 0; uCount < uWorkerCount; uCount++)
		m_ListOfWorkers.emplace_back(*this, std::bind(&ThreadPoolDynamic::_PoolCallback, this));
}

void ThreadPoolDynamic::_PoolCallback()
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