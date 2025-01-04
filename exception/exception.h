#pragma once
#include "extensions/constraints.h"
#include "utility/conversions.h"

#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <source_location>
#include <string>

#ifdef __cpp_lib_concepts

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
		_inner_processing(std::forward<_Fmt>(format), std::move(arguments));
	}

	template <janecekvit::constraints::format_view _Fmt, class... _Args>
	exception(std::source_location&& srcl, _Fmt&& format, _Args&&... arguments)
		: std::exception()
		, _srcl(std::move(srcl))
	{
		auto store = std::forward_as_tuple(arguments...);
		_inner_processing(std::forward<_Fmt>(format), std::move(store));
	}

	exception(std::source_location&& srcl = std::source_location::current())
		: std::exception()
		, _srcl(std::move(srcl))
	{
		_inner_processing("", {});
	}

	virtual ~exception() = default;

	[[nodiscard]] const char* what() const noexcept override
	{
		return _error.c_str();
	}

private:
	template <janecekvit::constraints::format_view _Fmt, class... _Args>
	void _inner_processing(_Fmt&& format, std::tuple<_Args...>&& arguments)
	{
		_error = _format_source_location();

		try
		{
			auto callback = [&](auto&&... arguments)
			{
				if constexpr (std::is_constructible_v<std::wstring_view, _Fmt>)
				{
					std::wstring errorWide = std::vformat(std::forward<_Fmt>(format), std::make_wformat_args(std::forward<decltype(arguments)>(arguments)...));
					_error += conversions::to_string(errorWide);
				}
				else
					_error += std::vformat(std::forward<_Fmt>(format), std::make_format_args(std::forward<decltype(arguments)>(arguments)...));
			};
			std::apply(callback, arguments);
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
		throw _Exception(std::move(format), std::forward_as_tuple(arguments...), std::move(srcl));
	}
};

template <janecekvit::constraints::format_view _Fmt, class... _Args>
throw_exception(_Fmt&&, _Args&&...) -> throw_exception<exception, _Fmt, _Args...>;

} // namespace janecekvit::exception

#endif
