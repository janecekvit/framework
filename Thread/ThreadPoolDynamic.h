#pragma once
#include <list>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>

#include "Framework/Thread/ThreadPool.h"
#include "Framework/Thread/WaitForMultipleConditions.h"

class ThreadPoolDynamic final
	: public virtual ThreadPool
{
public:
	/// <summary>
	/// Initializes a new instance of the <see cref="ThreadPoolDynamic"/> class.
	/// </summary>
	/// <param name="uiPoolSize">The minimum size of the thread pool. Cannot be less that this parameter</param>
	/// <param name="uDifference">The difference between size of queue and number of threads for incrasing number of threads.</param>
	ThreadPoolDynamic(size_t uiMinimumPoolSize, const double uDifference, WorkerErrorCallback&& fnCallback);
	virtual ~ThreadPoolDynamic();

public: // IThreadPool interface
	void AddTask(Task&& fn) noexcept override;

private:
	bool _IsDeallocationEnabled() const noexcept;
	bool _IsDeallocationFinished() const;

	/// <summary>
	/// Maine subroutine of thread pool.
	/// </summary>
	void _PoolController();
	
	void _DeallocateWorkers();

	/// <summary>
	/// Thread pool the callback for worker to call deallocation subrotine.
	/// </summary>
	bool _PoolCallback();



	enum class EConditionEvents : size_t
	{
		eNone,
		eExit,
		eMagnify,
		eReduce,
		eDealloc,
	};
	
	const double m_dDifference = 0;
	const size_t m_uMinimumPoolSize = 0;
	const size_t m_uPoolMultiplier = 2;
	const size_t m_uPoolDeallocationMultiplier = 8;

	std::thread m_oThread;
	std::atomic<size_t> m_uDeallocationPoolSize = 0;
	WaitForMultipleConditions<> m_cvDynamicPoolEvent;
	Concurrent::UnorderedSet<std::thread::id> m_oDeallocatedWorkers;
};