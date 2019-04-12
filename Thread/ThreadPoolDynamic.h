#pragma once
#include <list>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>
#include "WorkerDynamic.h"

class CWorkerDynamic;
class CThreadPoolDynamic
{
public:
	friend class CWorkerDynamic;

	/// <summary>
	/// Initializes a new instance of the <see cref="CThreadPoolDynamic"/> class.
	/// </summary>
	/// <param name="uiPoolSize">Size of the thread pool.</param>
	/// <param name="uDifference">The difference between size of queue and number of threads for incrasing number of threads.</param>
	CThreadPoolDynamic(_In_ const size_t uiPoolSize, _In_ const double uDifference);
	virtual ~CThreadPoolDynamic();
	
	/// <summary>
	/// Adds the task to working queue.
	/// </summary>
	/// <param name="fn">The function.</param>
	void AddTask(_In_ const std::function<void()> &fn);
	
	/// <summary>
	/// Gets the Pools the size.
	/// </summary>
	/// <returns>return size of the pool</returns>
	size_t GetPoolSize();

	
	/// <summary>
	/// Determines whether [is deallocation enabled].
	/// </summary>
	/// <returns>
	///   <c>true</c> if [is deallocation enabled]; otherwise, <c>false</c>.
	/// </returns>
	bool IsDeallocationEnabled() const { return m_uDeallocationPoolSize > 0; }

private:
#pragma region Fully Private Methods
	
	/// <summary>
	/// Maine subroutine of thread pool.
	/// </summary>
	void _PoolController();
	
	/// <summary>
	/// Adds the new workers to thread pool.
	/// </summary>
	/// <param name="uWorkerCount">The worker count.</param>
	void _AddWorkers(_In_ const size_t uWorkerCount);

	/// <summary>
	/// Thread pool the callback for worker to call deallocation subrotine.
	/// </summary>
	void _PoolCallback();

#pragma endregion 

#pragma region Friend Class Members
	enum class EConditionEvents : size_t
	{
		eNone = 0,
		eExit = 1,
		eMagnify = 2,
		eReduce = 3,
		eDealloc = 4,
	};
	
	std::atomic<size_t> m_uDeallocationPoolSize = 0;
	std::atomic<EConditionEvents> m_uConditionEvents = EConditionEvents::eNone;

	std::mutex m_mxtQueueLock;
	std::condition_variable m_cvQueueEvent;
	std::queue<std::function<void()>> m_queueTask;
#pragma endregion 


#pragma region Fully Private Members
	
	double m_dDifference = 0;
	size_t m_uMinimumPoolSize = 4;
	const size_t m_uPoolMultiplier = 2;
	const size_t m_uPoolDeallocationMultiplier = 8;

	std::mutex m_mxtPoolLock;
	std::condition_variable m_cvPoolEvent;
	std::condition_variable m_cvPoolEndEvent;


	std::thread *m_pCurrentThread = nullptr;
	std::list<CWorkerDynamic> m_ListOfWorkers;
#pragma endregion 
};