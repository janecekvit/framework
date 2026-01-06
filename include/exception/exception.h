#pragma once
#include "compatibility/compiler_support.h"
#include "extensions/constraints.h"
#include "utility/conversions.h"

#include <exception>
#include <iostream>
#include <memory>
#include <source_location>
#include <sstream>
#include <string>
#include <thread>

#if defined(HAS_STD_FORMAT)
#include <format>
#endif

namespace janecekvit::exception
{
class exception : public std::exception
{
public:

#if defined(HAS_STD_FORMAT)
	template <janecekvit::constraints::format_view _Fmt, class... _Args>
	exception(_Fmt&& format, std::tuple<_Args...> arguments = {}, std::source_location&& srcl = std::source_location::current(), std::thread::id&& thread = std::this_thread::get_id())
		: std::exception()
		, _srcl(std::move(srcl))
		, _thread(std::move(thread))
	{
		_inner_processing(std::forward<_Fmt>(format), std::move(arguments));
	}

	template <janecekvit::constraints::format_view _Fmt, class... _Args>
	exception(std::source_location&& srcl, std::thread::id&& thread, _Fmt&& format, _Args&&... arguments)
		: std::exception()
		, _srcl(std::move(srcl))
		, _thread(std::move(thread))
	{
		auto store = std::forward_as_tuple(std::forward<_Args>(arguments)...);
		_inner_processing(std::forward<_Fmt>(format), std::move(store));
	}

#else
	template <janecekvit::constraints::format_view _Fmt, class... _Args>
	exception(_Fmt&& message, std::source_location&& srcl = std::source_location::current(), std::thread::id&& thread = std::this_thread::get_id())
		: std::exception()
		, _srcl(std::move(srcl))
		, _thread(std::move(thread))
	{
		_inner_processing(std::forward<_Fmt>(message));
	}
#endif

	exception(std::source_location&& srcl = std::source_location::current(), std::thread::id&& thread = std::this_thread::get_id())
		: std::exception()
		, _srcl(std::move(srcl))
		, _thread(std::move(thread))
	{
#if defined(HAS_STD_FORMAT)
		_inner_processing("", {});
#else
		_inner_processing("");
#endif
	}


	virtual ~exception() = default;

	[[nodiscard]] const char* what() const noexcept override
	{
		return _error.c_str();
	}

	[[nodiscard]] const std::string& error() const noexcept
	{
		return _error;
	}

	[[nodiscard]] const std::source_location& source_location() const noexcept
	{
		return _srcl;
	}

	[[nodiscard]] const std::thread::id& thread_id() const noexcept
	{
		return _thread;
	}

private:
#if defined(HAS_STD_FORMAT)
	template <janecekvit::constraints::format_view _Fmt, class... _Args>
	void _inner_processing(_Fmt&& format, std::tuple<_Args...>&& arguments)
	{
		_error = _format_source_location();
		_error += _format_thread();
		try
		{
			if constexpr (std::is_constructible_v<std::wstring_view, _Fmt>)
			{
				std::wstring errorWide = std::apply([&format](auto&&... args)
					{
						return std::vformat(format, std::make_wformat_args(args...));
					},
					std::forward<decltype(arguments)>(arguments));
				_error += conversions::to_string(errorWide);
			}
			else
			{
				_error += std::apply([&format](auto&&... args)
					{
						return std::vformat(format, std::make_format_args(args...));
					},
					std::forward<decltype(arguments)>(arguments));
			}
		}
		catch (const std::exception& ex)
		{
			using namespace std::string_literals;
			_error += "Unexpected exception: "s + ex.what();
		}
	}
#else
	template <janecekvit::constraints::format_view _Fmt>
	void _inner_processing(_Fmt&& message)
	{
		_error = _format_source_location();
		_error += _format_thread();
		try
		{
			if constexpr (std::is_constructible_v<std::wstring_view, _Fmt>)
			{
				_error += conversions::to_string(message);
			}
			else
			{
				_error += message;
			}
		}
		catch (const std::exception& ex)
		{
			using namespace std::string_literals;
			_error += "Unexpected exception: "s + ex.what();
		}
	}
#endif

private:
	std::string _format_source_location() const
	{
#if defined(HAS_STD_FORMAT)
		return std::format("File: {}({}:{}) '{}'. ", _srcl.file_name(), _srcl.line(), _srcl.column(), _srcl.function_name());
#else
		std::ostringstream oss;
		oss << "File: " << _srcl.file_name() << "(" << _srcl.line() << ":" << _srcl.column() << ") '" << _srcl.function_name() << "'. ";
		return oss.str();
#endif
	}

	std::string _format_thread() const
	{
#if defined(HAS_STD_FORMAT) && defined(__cpp_lib_formatters)
		return std::format("Thread: {}. ", _thread);
#else
		std::ostringstream oss;
		oss << "Thread: " << _thread << ". ";
		return oss.str();
#endif
	}

private:
	std::string _error;
	std::source_location _srcl;
	std::thread::id _thread;
};

template <class _Exception, janecekvit::constraints::format_view _Fmt, class... _Args>
class throw_exception
{
public:
#if defined(HAS_STD_FORMAT)
	constexpr throw_exception(_Fmt&& format = {}, _Args&&... arguments, std::source_location&& srcl = std::source_location::current(), std::thread::id&& thread = std::this_thread::get_id())
	{
		throw _Exception(std::move(format), std::forward_as_tuple(arguments...), std::move(srcl), std::move(thread));
	}
#else
	constexpr throw_exception(_Fmt&& message, std::source_location&& srcl = std::source_location::current(), std::thread::id&& thread = std::this_thread::get_id())
	{
		throw _Exception(std::move(message), std::move(srcl), std::move(thread));
	}
#endif
};

template <janecekvit::constraints::format_view _Fmt, class... _Args>
throw_exception(_Fmt&&, _Args&&...) -> throw_exception<exception, _Fmt, _Args...>;

} // namespace janecekvit::exception
