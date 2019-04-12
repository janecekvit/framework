#pragma once
#include <thread>
#include <memory>

#include "ThreadPoolDynamic.h"

class CThreadPoolDynamic;
class CWorkerDynamic
{
public:

	friend class CThreadPoolDynamic;
	/// <summary>
	/// Initializes a new instance of the <see cref="CWorkerDynamic"/> class.
	/// </summary>
	/// <param name="pParentPool">The p parent pool.</param>
	/// <param name="pCallback">The p callback.</param>
	CWorkerDynamic(_Inout_ CThreadPoolDynamic *pParentPool, _In_ const std::function<void()> &pCallback);

	/// /// <summary>
	/// Finalizes an instance of the <see cref="CWorkerDynamic"/> class.
	/// </summary>
	virtual ~CWorkerDynamic();

#pragma region Friend Class Members
protected:
	/// <summary>
	/// Determines whether [is worker end].
	/// </summary>
	/// <returns>
	///   <c>true</c> if [is worker end]; otherwise, <c>false</c>.
	/// </returns>
	bool IsWorkerEnd() { return m_bWorkerEnd; }


	/// <summary>
	/// Joins this instance.
	/// </summary>
	void Join();
#pragma endregion 

private:
	/// <summary>
	/// Main working instance.
	/// </summary>
	void _Work();

	CThreadPoolDynamic  *m_pParentPool = nullptr;
	std::unique_ptr<std::thread> m_pCurrentThread = nullptr;

	std::atomic<bool> m_bWorkerEnd = false;
	std::function<void()> m_pCallback = nullptr;

};
