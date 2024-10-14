#include "thread/sync_thread_pool.h"

#include <thread>

namespace janecekvit::thread
{

sync_thread_pool::worker::worker(sync_thread_pool& parent)
{
//#ifdef __cpp_lib_jthread
//	_thread = std::jthread(&sync_thread_pool::_work, &parent);
//#else
	_thread = std::thread(&sync_thread_pool::_work, &parent);
//#endif
}

sync_thread_pool::worker::~worker()
{
//#ifndef __cpp_lib_jthread
	if (_thread.joinable())
		_thread.join();
//#endif
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
	{
		_event.signalize(state::Exit);
		
		// wait for the worker to exit
		for (; _exitedWorkers == position;)
			std::this_thread::yield();
	}
}

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

	_exitedWorkers++;
}

std::list<sync_thread_pool::worker> sync_thread_pool::_add_workers(size_t uWorkerCount) noexcept
{
	std::list<sync_thread_pool::worker> workers;
	for (size_t uCount = 0; uCount < uWorkerCount; uCount++)
		workers.emplace_back(*this);

	return workers;
}

std::optional<sync_thread_pool::_Task> sync_thread_pool::_get_task() noexcept
{
	// Wait for the signal and unblock queue until the signal will be received
	auto state = _event.wait();
	if (state == state::Exit)
		return {};

	auto&& scope	   = _tasks.exclusive();
	auto fnCurrentTask = std::move(scope->front());
	scope->pop();
	return fnCurrentTask;
}

} // namespace janecekvit::thread