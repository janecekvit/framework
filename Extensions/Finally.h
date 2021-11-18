#pragma once
#include "Extensions/constraints.h"

#include <functional>

namespace extensions
{
/// <summary>
/// finally class implements finally block in C++
/// </summary>

template <class _F, class... _Args>
class finally
{
public:
	template <std::enable_if_t<std::is_invocable_v<_F, _Args...>, int> = 0>
	finally(_F&& fnCallback)
		: _callback(std::move(fnCallback))
	{
	}

	virtual ~finally()
	{
		try
		{
			_callback();
		}
		catch (const std::exception&)
		{
		} //check double exception serious error
	}

private:
	const _F _callback{};
};

} //namespace extensions
