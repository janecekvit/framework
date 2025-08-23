#pragma once
#include "extensions/constraints.h"
#include "synchronization/concurrent.h"
#include "utility/conversions.h"

#include <format>
#include <source_location>
#include <thread>

namespace janecekvit::tracing
{

template <constraints::format_outout _FmtOutput, constraints::enum_type _Enum, constraints::format_view _Fmt, class... _Args>
class trace_event
{
public:
	trace_event(_Enum priority, _Fmt&& format, _Args&&... args, std::source_location&& srcl = std::source_location::current(), std::thread::id&& thread = std::this_thread::get_id())
		: _priority(std::move(priority))
		, _thread(std::move(thread))
		, _srcl(std::move(srcl))

	{
		try
		{
			if constexpr (constraints::format_wstring_view<_FmtOutput>)
				_data = std::vformat(std::move(format), std::make_wformat_args(args...));
			else
				_data = std::vformat(std::move(format), std::make_format_args(args...));
		}
		catch (const std::exception& ex)
		{
			using namespace std::string_literals;
			const std::vector<std::string> indexes = { std::type_index(typeid(_Args)).name()... };
			auto&& indexSerialized = conversions::to_string<std::string>(indexes);

			if constexpr (constraints::format_wstring_view<_FmtOutput>)
			{
				_data += L"Unexpected exception: "s + conversions::to_wstring(ex.what()) + L"\n"s + conversions::to_wstring(indexSerialized);
			}
			else
			{
				_data += "Unexpected exception: "s + ex.what() + "\n"s + indexSerialized;
			}
		}
	}

	virtual ~trace_event() = default;

	constexpr operator _Enum() const
	{
		return _priority;
	}

	operator const std::thread::id&() const&
	{
		return _thread;
	}

	operator std::thread::id() &&
	{
		return _thread;
	}

	operator const std::source_location&() const&
	{
		return _srcl;
	}

	operator std::source_location() &&
	{
		return _srcl;
	}

	constexpr operator const _FmtOutput&() const&
	{
		return _data;
	}

	constexpr operator _FmtOutput&&() &&
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

		constexpr _Enum priority() const
		{
			return _priority;
		}

		const std::thread::id& thread_id() const&
		{
			return _thread;
		}

		const std::source_location& source_location() const&
		{
			return _srcl;
		}

		const _Data& data() const&
		{
			return _data;
		}

		constexpr operator _Enum() const
		{
			return _priority;
		}

		operator const std::thread::id&() const&
		{
			return _thread;
		}

		operator std::thread::id&&() &&
		{
			return std::move(_thread);
		}

		operator const std::source_location&() const&
		{
			return _srcl;
		}

		operator std::source_location&&() &&
		{
			return std::move(_srcl);
		}

		operator const _Data&() const&
		{
			return _data;
		}

		operator _Data&&() &&
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

	virtual event next_trace()
	{
		auto&& scope = _traceQueue.exclusive();
		auto e = scope->front();
		scope->pop_front();
		return e;
	}

	virtual size_t size()
	{
		return _traceQueue.concurrent()->size();
	}

	virtual void flush()
	{
		_traceQueue.exclusive()->clear();
	}

protected:
	virtual void _process(event&& e)
	{
		_traceQueue.exclusive()->emplace_back(std::move(e));
	}

protected:
	synchronization::concurrent::deque<event> _traceQueue;
};

} // namespace janecekvit::tracing
