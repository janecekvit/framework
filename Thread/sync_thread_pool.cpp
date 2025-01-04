#include "thread/sync_thread_pool.h"

#include <thread>

namespace janecekvit::thread
{

sync_thread_pool::worker::worker(sync_thread_pool& parent)
{
#ifdef __cpp_lib_jthread
	_thread = std::jthread(&sync_thread_pool::_work, &parent);
#else
	_thread = std::thread(&sync_thread_pool::_work, &parent);
#endif
}

sync_thread_pool::worker::~worker()
{
	if (_thread.joinable())
		_thread.join();
}

sync_thread_pool::sync_thread_pool(size_t size)
	: _workers(_add_workers(size))
{
}

sync_thread_pool::~sync_thread_pool()
{
	for (size_t position = 0; position < _workers.size(); position++)
	{
		_event.signalize(state::Exit);

		// wait for the worker to exit
		for (; _exited_workers_count == position;)
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
		catch (const std::exception&)
		{
		}
	}

	_exited_workers_count++;
}

std::list<sync_thread_pool::worker> sync_thread_pool::_add_workers(size_t count) noexcept
{
	std::list<sync_thread_pool::worker> workers;
	for (size_t counter = 0; counter < count; counter++)
		workers.emplace_back(*this);

	return workers;
}

std::optional<sync_thread_pool::task> sync_thread_pool::_get_task() noexcept
{
	// Wait for the signal and unblock queue until the signal will be received
	auto state = _event.wait();
	if (state == state::Exit)
		return {};

	auto&& scope = _tasks.exclusive();
	auto fnCurrentTask = std::move(scope->front());
	scope->pop();
	return fnCurrentTask;
}

} // namespace janecekvit::thread
