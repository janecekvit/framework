#pragma once
#include "Sync/WaitForMultipleConditions.h"
#include "Thread/ThreadPool.h"

#include <atomic>
#include <functional>
#include <list>
#include <mutex>
#include <queue>

/// <summary>
///
/// Dynamic size thread pool that executes the task from the queue.
/// When the tasks queue is bigger than the multiplied thread by difference, the pool will be doubled.
/// Otherwise, when tasks queue is 8x smaller than current thread pool size, size is halved.
/// </summary>
class ThreadPoolDynamic final
	: public virtual ThreadPool
{
public:
	/// <summary>
	/// Initializes a new instance of the <see cref="ThreadPoolDynamic"/> class.
	/// </summary>
	/// <param name="uiPoolSize">The minimum size of the thread pool.</param>
	/// <param name="uDifference">The difference between the size of the queue and the number of threads for an increasing number of threads.</param>
	ThreadPoolDynamic(size_t uiMinimumPoolSize, const double uDifference, WorkerErrorCallback&& fnCallback);
	virtual ~ThreadPoolDynamic();

public: // IThreadPool interface
	void AddTask(Task&& fn) noexcept override;

private:
	bool _IsDeallocationEnabled() const noexcept;
	bool _IsDeallocationFinished() const;
	void _PoolController();
	void _DeallocateWorkers();
	bool _PoolCallback();

private:
	enum class EConditionEvents : size_t
	{
		eNone,
		eExit,
		eMagnify,
		eReduce,
		eDealloc,
	};

	const double m_dDifference				   = 0;
	const size_t m_uMinimumPoolSize			   = 0;
	const size_t m_uPoolMultiplier			   = 2;
	const size_t m_uPoolDeallocationMultiplier = 8;

	std::thread m_oThread;
	std::atomic<size_t> m_uDeallocationPoolSize = 0;
	WaitForMultipleConditions<> m_cvDynamicPoolEvent;
	concurrent::unordered_set<std::thread::id> m_oDeallocatedWorkers;
};