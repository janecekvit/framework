#pragma once
#include "extensions/constraints.h"
#include "synchronization/concurrent.h"
#include "utility/conversions.h"

#include <chrono>
#include <format>
#include <optional>
#include <source_location>
#include <thread>

namespace janecekvit::tracing
{

template <constraints::format_outout _FmtOutput, constraints::enum_type _Enum, constraints::format_view _Fmt, class... _Args>
class trace_event
{
public:
	trace_event(_Enum priority, _Fmt&& format, _Args&&... args, std::source_location srcl = std::source_location::current(), std::thread::id thread = std::this_thread::get_id())
		: _priority(priority)
		, _thread(thread)
		, _srcl(srcl)

	{
		try
		{
			if constexpr (constraints::format_wstring_view<_FmtOutput>)
				_data = std::vformat(format, std::make_wformat_args(args...));
			else
				_data = std::vformat(format, std::make_format_args(args...));
		}
		catch (const std::exception& ex)
		{
			using namespace std::string_literals;
			const std::vector<std::string> indexes = { std::type_index(typeid(_Args)).name()... };
			auto&& indexSerialized = conversions::to_string<std::string>(indexes);

			if constexpr (constraints::format_wstring_view<_FmtOutput>)
			{
				_data += L"Unexpected exception: ";
				_data += conversions::to_wstring(ex.what());
				_data += L"\nType arguments: ";
				_data += conversions::to_wstring(indexSerialized);
			}
			else
			{
				_data += "Unexpected exception: ";
				_data += ex.what();
				_data += "\nType arguments: ";
				_data += indexSerialized;
			}
		}
	}

	virtual ~trace_event() = default;

	constexpr operator _Enum() const noexcept
	{
		return _priority;
	}

	operator const std::thread::id&() const& noexcept
	{
		return _thread;
	}

	operator std::thread::id() && noexcept
	{
		return _thread;
	}

	operator const std::source_location&() const& noexcept
	{
		return _srcl;
	}

	operator std::source_location() && noexcept
	{
		return _srcl;
	}

	constexpr operator const _FmtOutput&() const& noexcept
	{
		return _data;
	}

	constexpr operator _FmtOutput&&() && noexcept
	{
		return std::move(_data);
	}

private:
	_Enum _priority;
	std::thread::id _thread;
	std::source_location _srcl;
	_FmtOutput _data;
};

/// <summary>
/// User-defined deduction guide CTAD for trace_event using std::string as input format.
/// </summary>
template <constraints::enum_type _Enum, constraints::format_view _Fmt, class... _Args>
	requires constraints::format_string_view<_Fmt>
trace_event(_Enum, _Fmt&&, _Args&&...)
	-> trace_event<std::string, _Enum, _Fmt, _Args...>;

/// <summary>
/// User-defined deduction guide CTAD for trace_event using std::wstring as input format.
/// </summary>
template <constraints::enum_type _Enum, constraints::format_view _Fmt, class... _Args>
	requires constraints::format_wstring_view<_Fmt>
trace_event(_Enum, _Fmt&&, _Args&&...)
	-> trace_event<std::wstring, _Enum, _Fmt, _Args...>;

template <constraints::format_outout _Data, constraints::enum_type _Enum>
class trace
{
	class event
	{
	public:
		template <constraints::format_view _Fmt, class... _Args>
		event(trace_event<_Data, _Enum, _Fmt, _Args...>&& e)
			: _priority(e)
			, _thread(e)
			, _srcl(e)
			, _data(e)
		{
		}

		constexpr _Enum priority() const noexcept
		{
			return _priority;
		}

		const std::thread::id& thread_id() const& noexcept
		{
			return _thread;
		}

		const std::source_location& source_location() const& noexcept
		{
			return _srcl;
		}

		const _Data& data() const& noexcept
		{
			return _data;
		}

		constexpr operator _Enum() const noexcept
		{
			return _priority;
		}

		operator const std::thread::id&() const& noexcept
		{
			return _thread;
		}

		operator std::thread::id&&() && noexcept
		{
			return std::move(_thread);
		}

		operator const std::source_location&() const& noexcept
		{
			return _srcl;
		}

		operator std::source_location&&() && noexcept
		{
			return std::move(_srcl);
		}

		operator const _Data&() const& noexcept
		{
			return _data;
		}

		operator _Data&&() && noexcept
		{
			return std::move(_data);
		}

	private:
		_Enum _priority;
		std::thread::id _thread;
		std::source_location _srcl;
		_Data _data;
	};

public:
	virtual ~trace() = default;

	template <constraints::format_view _Fmt, class... _Args>
	void create(trace_event<_Data, _Enum, _Fmt, _Args...>&& value)
	{
		_process(event(std::move(value)));
	}

	[[nodiscard]] virtual std::optional<event> get_next_trace()
	{
		auto&& scope = _traceQueue.exclusive();
		if (scope->empty())
			return std::nullopt;

		auto e = std::move(scope->front());
		scope->pop_front();
		return e;
	}

	[[nodiscard]] virtual event get_next_trace_wait()
	{
		auto&& scope = _traceQueue.exclusive();
		scope.wait(_event, [&]
			{
				return !scope->empty();
			});

		auto e = std::move(scope->front());
		scope->pop_front();
		return e;
	}

	template <class _Rep, class _Period>
	[[nodiscard]] std::optional<event> get_next_trace_wait_for(const std::chrono::duration<_Rep, _Period>& timeout)
	{
		auto&& scope = _traceQueue.exclusive();

		if (!scope.wait_for(_event, timeout, [&]
				{
					return !scope->empty();
				}))
			return std::nullopt;

		auto e = std::move(scope->front());
		scope->pop_front();
		return e;
	}

	[[nodiscard]] virtual size_t size() const
	{
		return _traceQueue.exclusive()->size();
	}

	virtual void flush()
	{
		_traceQueue.exclusive()->clear();
	}

protected:
	virtual void _process(event&& e)
	{
		_traceQueue.exclusive()->emplace_back(std::move(e));
		_event.notify_one();
	}

protected:
	std::condition_variable_any _event;
	mutable synchronization::concurrent::deque<event> _traceQueue;
};

} // namespace janecekvit::tracing
