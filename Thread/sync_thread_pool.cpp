#include "thread/sync_thread_pool.h"

#include <thread>

namespace janecekvit::thread
{

template <class... Args>
inline sync_thread_pool::worker::worker(sync_thread_pool& parent)
{
	_thread = std::thread(&sync_thread_pool::_work, parent);
}

sync_thread_pool::worker::~worker()
{
	if (_thread.joinable())
		_thread.join();
}

sync_thread_pool::sync_thread_pool(size_t uiPoolSize)
	: _workers(_add_workers(uiPoolSize))
{
}

sync_thread_pool::sync_thread_pool(size_t uiPoolSize, _ErrorCallback&& callback)
	: _workers(_add_workers(uiPoolSize))
	, _callback(callback)
{
}

sync_thread_pool::~sync_thread_pool()
{
	for (size_t position = 0; position < _workers.size(); position++)
		_event.signalize(state::Exit);
}

// void sync_thread_pool::AddTask(Task&& fn) noexcept
//{
//	if (m_bEndFlag)
//		return;
//
//	// Add new task to queue and inform workers about it
//	m_queueTask.exclusive()->emplace(std::move(fn));
//	m_cvPoolEvent.notify_one();
// }
//
// void sync_thread_pool::WaitAll() const noexcept
//{
//	auto&& oScope = m_queueTask.concurrent();
//	oScope.wait(_WaitEvent(), [&oScope, this]()
//		{
//			return oScope->empty() || _Exit();
//		});
// }
//
size_t janecekvit::thread::sync_thread_pool::size() const noexcept
{
	return _tasks.concurrent()->size();
}

size_t sync_thread_pool::pool_size() const noexcept
{
	return _workers.size();
}

void sync_thread_pool::_work()
{
	for (auto&& fnCurrentTask = _get_task(); fnCurrentTask; fnCurrentTask = _get_task())
	{
		try
		{ // execute task
			(*fnCurrentTask)();
		}
		catch (const std::exception& ex)
		{
			if (_callback)
				_callback.value()(ex);
		}
	}
}

std::list<std::thread> sync_thread_pool::_add_workers(size_t uWorkerCount) noexcept
{
	std::list<std::thread> workers;
	for (size_t uCount = 0; uCount < uWorkerCount; uCount++)
		workers.emplace_back(&sync_thread_pool::_work, this);

	return workers;
}

std::optional<sync_thread_pool::_Task> sync_thread_pool::_get_task() noexcept
{
	// Wait for the signal and unblock queue until the signal will be received
	auto state = _event.wait();
	if (state == state::Exit)
		return {};

	auto&& scope	   = _tasks.exclusive();
	auto fnCurrentTask = scope->front();
	scope->pop();
	return fnCurrentTask;
}

} // namespace janecekvit::thread