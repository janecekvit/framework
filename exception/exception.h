#pragma once
#include "extensions/constraints.h"
#include "utility/conversions.h"

#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <source_location>
#include <string>

namespace janecekvit::exception
{
class exception : public std::exception
{
public:
	template <janecekvit::constraints::format_view _Fmt, class... _Args>
	exception(_Fmt&& format, std::tuple<_Args...> arguments, std::source_location&& srcl = std::source_location::current())
		: std::exception()
		, _srcl(std::move(srcl))
	{
		auto callback = [&](auto&&... args)
		{
			return _inner_processing(std::move(srcl), std::forward<_Fmt>(format), std::forward<decltype(args)>(args)...);
		};
		std::apply(callback, arguments);
	}

	template <janecekvit::constraints::format_view _Fmt, class... _Args>
	exception(std::source_location&& srcl, _Fmt&& format, _Args&&... arguments)
		: std::exception()
		, _srcl(std::move(srcl))
	{
		_inner_processing(std::move(srcl), std::forward<_Fmt>(format), std::forward<_Args>(arguments)...);
	}

	exception(std::source_location&& srcl = std::source_location::current())
		: std::exception()
		, _srcl(std::move(srcl))
	{
		_inner_processing(std::move(srcl), "");
	}

	virtual ~exception() = default;

	[[nodiscard]] const char* what() const noexcept override
	{
		return _error.c_str();
	}

private:
	template <janecekvit::constraints::format_view _Fmt, class... _Args>
	void _inner_processing(std::source_location&& srcl, _Fmt&& format, _Args&&... arguments)
	{
		_error = _format_source_location();

		auto callback = [&](auto&&... args)
		{
			return std::format(format, std::forward<decltype(args)>(args)...);
		};

		try
		{
			if constexpr (std::is_constructible_v<std::wstring_view, _Fmt>)
			{
				std::wstring _error_wide = std::invoke(callback, arguments...);
				_error += conversions::to_string(_error_wide);
			}
			else
				_error += std::invoke(callback, arguments...);
		}
		catch (const std::exception& ex)
		{
			using namespace std::string_literals;
			_error += "Unexpected exception: "s + ex.what();
		}
	}

private:
	std::string _format_source_location() const
	{
		return std::format("File: {}({}:{}) '{}'. ", _srcl.file_name(), _srcl.line(), _srcl.column(), _srcl.function_name());
	}

private:
	std::string _error;
	std::source_location _srcl;
};

template <class _Exception, janecekvit::constraints::format_view _Fmt, class... _Args>
class throw_exception
{
public:
	constexpr throw_exception(_Fmt&& format = {}, _Args&&... arguments, std::source_location&& srcl = std::source_location::current())
	{
		throw _Exception(format, std::forward_as_tuple(arguments...), std::move(srcl));
	}
};

template <janecekvit::constraints::format_view _Fmt, class... _Args>
throw_exception(_Fmt&&, _Args&&...) -> throw_exception<exception, _Fmt, _Args...>;

} // namespace janecekvit::exception