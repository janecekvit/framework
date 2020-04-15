#pragma once
#include <functional>
#include "Framework/Extensions/Constraints.h"

namespace Extensions
{

/// <summary>
/// Finally class implements finally block in C++
/// </summary>

template <class Func, class ... Args>
class Finally 
{
public:
	template <std::enable_if_t<std::is_invocable_v<Func, Args...>, int> = 0>
	Finally(Func&& fnCallback)
		: m_fnCallback(std::move(fnCallback))
	{
	}

	virtual ~Finally()
	{
		try
		{
			m_fnCallback();
		}
		catch (const std::exception& ) 
		{

		} //check double exception serious error
		
	}

private:
	const Func m_fnCallback {};
};

} //namespace Extensions
