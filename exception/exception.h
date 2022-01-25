#pragma once
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <source_location>
#include <string>

//C++20
#include "extensions/constraints.h"

namespace janecekvit::exception
{
template <class... _Args>
class builder
{
	builder(_Args&... args)
		: _args(std::forward<_Args>(args)...)
	{
	}

private:
	std::tuple<_Args...> _args;
};

class exception : public std::exception
{
public:
	template <class _Fmt, class... _Args>
	exception(_Fmt&& format, std::tuple<_Args...>&& arguments, std::source_location&& srcl = std::source_location::current())
		: std::exception()
		, _srcl(std::move(srcl))
	{
		_error = std::format("File: {}({}:{}) '{}'. ", _srcl.file_name(), _srcl.line(), _srcl.column(), _srcl.function_name());

		try
		{
			if constexpr (std::is_same_v<_Fmt, std::wstring>)
			{
				std::wstring _error_wide = std::format(format, arguments);
				_error += std::string{ std::begin(format), std::end(format) };
			}
			else
				_error += std::format(format, std::forward<_Args>(arguments)...);
		}
		catch (const std::exception& ex)
		{
			using namespace std::string_literals;
			_error += "Unexpected exception: "s + ex.what();
		}
	}

	virtual ~exception() = default;

	[[nodiscard]] const char* what() const noexcept override
	{
		return _error.c_str();
	}

private:
	std::string _error;
	std::source_location _srcl;
};
} // namespace janecekvit::exception