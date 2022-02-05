#pragma once
#include "extensions/constraints.h"

#include <source_location>
#include <thread>

namespace janecekvit::tracing
{
template <constraints::format_outout _FmtOutput, constraints::enum_type _Enum, constraints::format_view _Fmt, class... _Args>
class trace_event
{
public:
	trace_event(_Enum priority, std::thread::id&& thread, _Fmt&& format, _Args&&... args, std::source_location&& srcl = std::source_location::current())
		: _priority(std::move(priority))
		, _thread(std::move(thread))
		, _srcl(std::move(srcl))

	{
		try
		{
			_data = std::format(format, std::forward<_Args>(args)...);
		}
		catch (const std::exception& ex)
		{
			/*using namespace std::string_literals;
			auto indexes = std::type_index(typeid(_Args));
			_data += L"Unexpected exception: "s + ex.what();*/
		}
	}
	virtual ~trace_event() = default;

private:
	const _Enum _priority;
	const std::thread::id _thread;
	const std::source_location _srcl;
	_FmtOutput _data;
};

template <constraints::enum_type _Enum, constraints::format_view _Fmt, class... _Args>
requires constraints::format_string_view<_Fmt>
trace_event(_Enum, std::thread::id&&, _Fmt&&, _Args&&...)
->trace_event<std::string, _Enum, _Fmt, _Args...>;

template <constraints::enum_type _Enum, constraints::format_view _Fmt, class... _Args>
requires constraints::format_wstring_view<_Fmt>
trace_event(_Enum, std::thread::id&&, _Fmt&&, _Args&&...)
->trace_event<std::wstring, _Enum, _Fmt, _Args...>;

template <constraints::format_outout _Data, constraints::enum_type _Enum>
class trace
{
	class event
	{
	public:
		template <constraints::format_view _Fmt, class... _Args>
		event(trace_event<_Enum, _Fmt, _Args...>&& e)
		{
			//e.
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
	void create(trace_event<_Enum, _Fmt, _Args...>&& value)
	{
		event e(std::move(value));
	}

protected:
	//virtual void process(event&& event) = 0;

protected:
};

} // namespace janecekvit::tracing