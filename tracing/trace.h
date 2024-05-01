#pragma once
#include "extensions/concurrent.h"
#include "extensions/constraints.h"
#include "utility/conversions.h"

#include <format>
#include <source_location>
#include <thread>

#if __cplusplus >= __cpp_lib_concepts

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
				_data = std::vformat(format, std::make_wformat_args(args...));
			else
				_data = std::vformat(format, std::make_format_args(args...));
		}
		catch (const std::exception& ex)
		{
			using namespace std::string_literals;
			const std::vector<std::string> indexes = { std::type_index(typeid(_Args)).name()... };
			auto&& index_serialized				   = conversions::to_string<std::string>(indexes);

			if constexpr (constraints::format_wstring_view<_FmtOutput>)
			{
				_data += L"Unexpected exception: "s + conversions::to_wstring(ex.what()) + L"\n"s + conversions::to_wstring(index_serialized);
			}
			else
			{
				_data += "Unexpected exception: "s + ex.what() + "\n"s + index_serialized;
			}
		}
	}

	virtual ~trace_event() = default;

	constexpr operator _Enum() const
	{
		return _priority;
	}

	constexpr operator const std::thread::id&() const&
	{
		return _thread;
	}

	constexpr operator std::thread::id&&() &&
	{
		return _thread;
	}

	constexpr operator const std::source_location&() const&
	{
		return _srcl;
	}

	constexpr operator std::source_location&&() &&
	{
		return std::move(_srcl);
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
	const _Enum _priority;
	const std::thread::id _thread;
	const std::source_location _srcl;
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
		{
			_priority = e;
			_thread	  = e;
			_srcl	  = e;
			_data	  = e;
		}

		constexpr _Enum priority() const
		{
			return _priority;
		}

		constexpr const std::thread::id& thread_id() const&
		{
			return _thread;
		}

		constexpr const std::source_location& source_location() const&
		{
			return _srcl;
		}

		constexpr const _Data& data() const&
		{
			return _data;
		}

		constexpr operator _Enum() const
		{
			return _priority;
		}

		constexpr operator const std::thread::id&() const&
		{
			return _thread;
		}

		constexpr operator std::thread::id&&() &&
		{
			return _thread;
		}

		constexpr operator const std::source_location&() const&
		{
			return _srcl;
		}

		constexpr operator std::source_location&&() &&
		{
			return std::move(_srcl);
		}

		constexpr operator const _Data&() const&
		{
			return _data;
		}

		constexpr operator _Data&&() &&
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
		process(event(std::move(value)));
	}

	virtual event next_trace()
	{
		auto&& scope = _traceQueue.exclusive();
		auto e		 = scope->front();
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
	virtual void process(event&& e)
	{
		_traceQueue.exclusive()->emplace_back(std::move(e));
	}

protected:
	concurrent::deque<event> _traceQueue;
};

} // namespace janecekvit::tracing

#endif