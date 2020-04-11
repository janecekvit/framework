#pragma once
#include <optional>
#include <functional>

class IThreadPool
{
public:
	using Task = typename std::function<void()>;
	using WorkerErrorCallback = typename std::function<void(const std::exception&)>;

public:
	IThreadPool() = default;
	virtual ~IThreadPool() = default;

	virtual void AddTask(Task&& fn) noexcept = 0;

private:
	const WorkerErrorCallback m_fnErrorCallback;
};