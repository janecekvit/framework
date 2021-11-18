#include "ThreadPoolDynamic.h"

ThreadPoolDynamic::ThreadPoolDynamic(size_t uiPoolSize, const double uDifference, WorkerErrorCallback&& fnCallback)
	: ThreadPool(uiPoolSize, std::move(fnCallback), [this]()
		  {
			  return _PoolCallback();
		  })
	, m_dDifference(uDifference)
	, m_uMinimumPoolSize(uiPoolSize)
{
	m_oThread = std::thread(&ThreadPoolDynamic::_PoolController, this);
}

ThreadPoolDynamic::~ThreadPoolDynamic()
{
	//Set End Flag and Notify Finish
	_SetExit();

	m_cvDynamicPoolEvent.notify_one(static_cast<size_t>(EConditionEvents::eExit));

	if (m_oThread.joinable())
		m_oThread.join();
}

void ThreadPoolDynamic::AddTask(Task&& fn) noexcept
{
	if (_Exit())
		return;

	size_t uCurrentQueueSize = Size();
	size_t uCurrentPoolSize	 = PoolSize();

	//Check if Task _Queue is bigger that multiplier of the list of workers -> increase thread pool.
	if (uCurrentQueueSize > m_dDifference * uCurrentPoolSize)
		m_cvDynamicPoolEvent.notify_one(static_cast<size_t>(EConditionEvents::eMagnify));

	//Check if Task _Queue is smaller than the minimum pool size and smaller that multiplier of the list of workers.
	else if ((uCurrentPoolSize > m_uMinimumPoolSize) && ((uCurrentQueueSize * m_uPoolDeallocationMultiplier) < uCurrentPoolSize))
		m_cvDynamicPoolEvent.notify_one(static_cast<size_t>(EConditionEvents::eReduce));

	//Call base task
	ThreadPool::AddTask(std::move(fn));
}

bool ThreadPoolDynamic::_IsDeallocationEnabled() const noexcept
{
	return m_uDeallocationPoolSize > 0;
}

bool ThreadPoolDynamic::_IsDeallocationFinished() const
{
	return m_oDeallocatedWorkers.concurrent().size() == 0; //TODO
}

void ThreadPoolDynamic::_PoolController()
{
	std::mutex mxtPoolLock;
	std::unique_lock<std::mutex> unqPoolLock(mxtPoolLock);

	for (;;)
	{
		for (auto&& uEvent : m_cvDynamicPoolEvent.wait(unqPoolLock))
		{
			auto&& eEvent = static_cast<EConditionEvents>(uEvent);
			if (eEvent == EConditionEvents::eExit)
				return;

			if (eEvent == EConditionEvents::eMagnify)
			{
				if (!_IsDeallocationEnabled() && _IsDeallocationFinished())
				{
					_AddWorkers(PoolSize(), [this]()
						{
							return _PoolCallback();
						});
				}
			}
			else if (eEvent == EConditionEvents::eReduce)
			{
				if (!_IsDeallocationEnabled() && _IsDeallocationFinished())
					m_uDeallocationPoolSize = PoolSize() / m_uPoolMultiplier;
			}

			else if (eEvent == EConditionEvents::eDealloc)
			{
				_DeallocateWorkers();
				m_oDeallocatedWorkers.exclusive()->clear();
			}
		}
	}
}

void ThreadPoolDynamic::_DeallocateWorkers()
{
	//release workers
	auto&& oScope		 = _Pool().exclusive();
	auto&& oDeallocation = m_oDeallocatedWorkers.concurrent();
	for (std::list<Worker>::iterator it = oScope->begin(); it != oScope->end();)
	{
		//Deallocated worker -> Join worker and release resources
		if (oDeallocation->find(it->Id()) != oDeallocation->end())
			oScope->erase(it++);
		else //Or Get next
			++it;
	}
}

bool ThreadPoolDynamic::_PoolCallback()
{
	if (_Exit())
		return true;

	if (!_IsDeallocationEnabled())
		return false;

	//Decrement reference counter and save thread id
	m_uDeallocationPoolSize--;
	m_oDeallocatedWorkers.exclusive()->emplace(std::this_thread::get_id());

	//If zero references
	if (!_IsDeallocationEnabled())
	{
		//Notify controller to delete memory (Garbage Collector)
		m_cvDynamicPoolEvent.notify_one(static_cast<size_t>(EConditionEvents::eDealloc));
	}

	return true;
}