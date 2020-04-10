#pragma once
#include <list>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>

class ThreadPoolDynamic
{

public:
	class Worker
	{
	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="Worker"/> class.
		/// </summary>
		/// <param name="pParentPool">The p parent pool.</param>
		/// <param name="pCallback">The p callback.</param>
		Worker(ThreadPoolDynamic& oParentPool, _In_ const std::function<void()>& pCallback);

		/// /// <summary>
		/// Finalizes an instance of the <see cref="Worker"/> class.
		/// </summary>
		virtual ~Worker();

		/// <summary>
		/// Determines whether [is worker end].
		/// </summary>
		/// <returns>
		///   <c>true</c> if [is worker end]; otherwise, <c>false</c>.
		/// </returns>
		bool IsWorkerEnd()
		{
			return m_bWorkerEnd;
		}

	private:
		/// <summary>
		/// Main working instance.
		/// </summary>
		void _Work();


		ThreadPoolDynamic& m_oParentPool; //dependency
		std::thread m_oThread;

		std::atomic<bool> m_bWorkerEnd = false;
		std::function<void()> m_pCallback = nullptr;

	};


public:
	/// <summary>
	/// Initializes a new instance of the <see cref="ThreadPoolDynamic"/> class.
	/// </summary>
	/// <param name="uiPoolSize">Size of the thread pool.</param>
	/// <param name="uDifference">The difference between size of queue and number of threads for incrasing number of threads.</param>
	ThreadPoolDynamic(_In_ const size_t uiPoolSize, _In_ const double uDifference);
	virtual ~ThreadPoolDynamic();
	
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
	std::list<Worker> m_ListOfWorkers;
#pragma endregion 
};